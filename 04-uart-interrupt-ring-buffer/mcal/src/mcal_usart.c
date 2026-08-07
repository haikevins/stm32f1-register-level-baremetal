#include "mcal_usart.h"

#include <stddef.h>

#include "cortex_m3.h"
#include "cortex_m3_registers.h"
#include "mcal_config.h"
#include "stm32f103xb.h"
#include "stm32f103xb_irq.h"
#include "stm32f103xb_register_bits.h"

#define MCAL_USART_RX_INDEX_MASK \
    ((uint16_t)(MCAL_USART_RX_BUFFER_SIZE - 1UL))
#define MCAL_USART_TX_INDEX_MASK \
    ((uint16_t)(MCAL_USART_TX_BUFFER_SIZE - 1UL))

typedef struct
{
    volatile uint8_t rx_buffer[MCAL_USART_RX_BUFFER_SIZE];
    volatile uint8_t tx_buffer[MCAL_USART_TX_BUFFER_SIZE];

    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;

    volatile uint32_t error_flags;
    volatile uint32_t rx_overflow_count;
} mcal_usart_state_t;

static mcal_usart_state_t g_usart_state[MCAL_USART_INSTANCE_COUNT];

static stm32_usart_registers_t *usart_registers(
    mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return STM32_USART1;
        default:
            return NULL;
    }
}

static uint32_t usart_clock_mask(mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return STM32_RCC_APB2ENR_USART1EN;
        default:
            return 0U;
    }
}

static stm32_irq_number_t usart_irq_number(
    mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return USART1_IRQn;
        default:
            return NonMaskableInt_IRQn;
    }
}

static uint8_t usart_irq_priority(mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return (uint8_t)MCAL_USART1_IRQ_PRIORITY;
        default:
            return 15U;
    }
}

static uint16_t next_rx_index(uint16_t index)
{
    return (uint16_t)(((uint32_t)index + 1UL) &
                      (uint32_t)MCAL_USART_RX_INDEX_MASK);
}

static uint16_t next_tx_index(uint16_t index)
{
    return (uint16_t)(((uint32_t)index + 1UL) &
                      (uint32_t)MCAL_USART_TX_INDEX_MASK);
}

static uint32_t portable_error_flags(uint32_t status)
{
    uint32_t flags = 0U;

    if ((status & STM32_USART_SR_PE) != 0U)
    {
        flags |= MCAL_USART_ERROR_PARITY;
    }

    if ((status & STM32_USART_SR_FE) != 0U)
    {
        flags |= MCAL_USART_ERROR_FRAMING;
    }

    if ((status & STM32_USART_SR_NE) != 0U)
    {
        flags |= MCAL_USART_ERROR_NOISE;
    }

    if ((status & STM32_USART_SR_ORE) != 0U)
    {
        flags |= MCAL_USART_ERROR_OVERRUN;
    }

    return flags;
}

static uint32_t enter_critical_section(void)
{
    const uint32_t primask = cortex_m3_get_primask();
    cortex_m3_disable_irq();
    return primask;
}

static void exit_critical_section(uint32_t primask)
{
    if ((primask & 1U) == 0U)
    {
        cortex_m3_enable_irq();
    }
}

static void nvic_enable_irq(stm32_irq_number_t irq_number,
                            uint8_t priority)
{
    const uint32_t irq = (uint32_t)irq_number;
    const uint32_t register_index = irq / 32UL;
    const uint32_t bit_index = irq % 32UL;

    CORTEX_M3_NVIC->ICER[register_index] =
        UINT32_C(1) << bit_index;
    CORTEX_M3_NVIC->ICPR[register_index] =
        UINT32_C(1) << bit_index;

    /*
     * STM32F1 implements the upper four priority bits.
     */
    CORTEX_M3_NVIC->IP[irq] = (uint8_t)((uint32_t)priority << 4U);
    CORTEX_M3_NVIC->ISER[register_index] =
        UINT32_C(1) << bit_index;
}

static void reset_state(mcal_usart_state_t *state)
{
    state->rx_head = 0U;
    state->rx_tail = 0U;
    state->tx_head = 0U;
    state->tx_tail = 0U;
    state->error_flags = 0U;
    state->rx_overflow_count = 0U;
}

bool mcal_usart_init(mcal_usart_instance_t instance,
                     uint32_t peripheral_clock_hz,
                     uint32_t baud_rate)
{
    stm32_usart_registers_t *usart = usart_registers(instance);
    const uint32_t clock_mask = usart_clock_mask(instance);
    const stm32_irq_number_t irq_number = usart_irq_number(instance);

    if ((usart == NULL) ||
        (clock_mask == 0U) ||
        ((int32_t)irq_number < 0) ||
        (peripheral_clock_hz == 0U) ||
        (baud_rate == 0U))
    {
        return false;
    }

    /*
     * With oversampling by 16, BRR is approximately PCLK / baud.
     */
    const uint32_t baud_divider =
        (peripheral_clock_hz + (baud_rate / 2U)) / baud_rate;

    if ((baud_divider < 16U) || (baud_divider > UINT32_C(0xFFFF)))
    {
        return false;
    }

    STM32_RCC->APB2ENR |= clock_mask;
    (void)STM32_RCC->APB2ENR;

    usart->CR1 = 0U;
    usart->CR2 = 0U;
    usart->CR3 = 0U;
    usart->BRR = baud_divider;

    reset_state(&g_usart_state[instance]);

    /*
     * RXNEIE also reports overrun conditions on STM32F1. TXEIE remains
     * disabled until the thread queues the first byte.
     */
    usart->CR1 = STM32_USART_CR1_TE |
                 STM32_USART_CR1_RE |
                 STM32_USART_CR1_RXNEIE |
                 STM32_USART_CR1_UE;

    nvic_enable_irq(irq_number, usart_irq_priority(instance));
    return true;
}

bool mcal_usart_try_read_byte(mcal_usart_instance_t instance, uint8_t *byte)
{
    if (((uint32_t)instance >= (uint32_t)MCAL_USART_INSTANCE_COUNT) ||
        (byte == NULL))
    {
        return false;
    }

    mcal_usart_state_t *state = &g_usart_state[instance];
    const uint16_t tail = state->rx_tail;

    if (tail == state->rx_head)
    {
        return false;
    }

    *byte = state->rx_buffer[tail];
    state->rx_tail = next_rx_index(tail);
    return true;
}

bool mcal_usart_try_write_byte(mcal_usart_instance_t instance, uint8_t byte)
{
    stm32_usart_registers_t *usart = usart_registers(instance);

    if (usart == NULL)
    {
        return false;
    }

    mcal_usart_state_t *state = &g_usart_state[instance];
    const uint32_t primask = enter_critical_section();
    const uint16_t head = state->tx_head;
    const uint16_t next_head = next_tx_index(head);

    if (next_head == state->tx_tail)
    {
        exit_critical_section(primask);
        return false;
    }

    state->tx_buffer[head] = byte;
    state->tx_head = next_head;

    /*
     * Enqueue and TXEIE enable are atomic relative to the ISR. This prevents
     * the empty-queue ISR path from clearing TXEIE after a producer has just
     * queued a new byte.
     */
    usart->CR1 |= STM32_USART_CR1_TXEIE;

    exit_critical_section(primask);
    return true;
}

uint32_t mcal_usart_take_error_flags(mcal_usart_instance_t instance)
{
    if ((uint32_t)instance >= (uint32_t)MCAL_USART_INSTANCE_COUNT)
    {
        return 0U;
    }

    mcal_usart_state_t *state = &g_usart_state[instance];
    const uint32_t primask = enter_critical_section();
    const uint32_t flags = state->error_flags;
    state->error_flags = 0U;
    exit_critical_section(primask);
    return flags;
}

uint32_t mcal_usart_take_rx_overflow_count(
    mcal_usart_instance_t instance)
{
    if ((uint32_t)instance >= (uint32_t)MCAL_USART_INSTANCE_COUNT)
    {
        return 0U;
    }

    mcal_usart_state_t *state = &g_usart_state[instance];
    const uint32_t primask = enter_critical_section();
    const uint32_t count = state->rx_overflow_count;
    state->rx_overflow_count = 0U;
    exit_critical_section(primask);
    return count;
}

static void handle_rx_interrupt(stm32_usart_registers_t *usart,
                                mcal_usart_state_t *state,
                                uint32_t status)
{
    const uint32_t hardware_errors =
        status & STM32_USART_SR_ERROR_MASK;

    if ((hardware_errors == 0U) &&
        ((status & STM32_USART_SR_RXNE) == 0U))
    {
        return;
    }

    /*
     * Reading SR followed by DR clears RXNE and PE/FE/NE/ORE on STM32F1.
     */
    const uint8_t byte =
        (uint8_t)(usart->DR & UINT32_C(0xFF));

    if (hardware_errors != 0U)
    {
        state->error_flags |= portable_error_flags(hardware_errors);
        return;
    }

    const uint16_t head = state->rx_head;
    const uint16_t next_head = next_rx_index(head);

    if (next_head == state->rx_tail)
    {
        state->rx_overflow_count++;
        return;
    }

    state->rx_buffer[head] = byte;
    state->rx_head = next_head;
}

static void handle_tx_interrupt(stm32_usart_registers_t *usart,
                                mcal_usart_state_t *state,
                                uint32_t status)
{
    if (((usart->CR1 & STM32_USART_CR1_TXEIE) == 0U) ||
        ((status & STM32_USART_SR_TXE) == 0U))
    {
        return;
    }

    const uint16_t tail = state->tx_tail;

    if (tail == state->tx_head)
    {
        usart->CR1 &= ~STM32_USART_CR1_TXEIE;
        return;
    }

    usart->DR = state->tx_buffer[tail];
    state->tx_tail = next_tx_index(tail);
}

static void mcal_usart_handle_interrupt(
    mcal_usart_instance_t instance)
{
    stm32_usart_registers_t *usart = usart_registers(instance);

    if (usart == NULL)
    {
        return;
    }

    mcal_usart_state_t *state = &g_usart_state[instance];
    const uint32_t status = usart->SR;

    handle_rx_interrupt(usart, state, status);
    handle_tx_interrupt(usart, state, status);
}

void USART1_IRQHandler(void)
{
    mcal_usart_handle_interrupt(MCAL_USART_INSTANCE_1);
}
