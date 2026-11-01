#include "mcal_can.h"
#include <stddef.h>
#include "stm32f103xb.h"
#include "stm32f103xb_can.h"

#define CAN_SAMPLE_TQ (16U)
#define CAN_TS1 (13U)
#define CAN_TS2 (2U)
#define CAN_SJW (1U)
#define CAN_BRP_MAX (1024U)
#define CAN_STD_ID_MAX (0x7FFU)
#define CAN_DLC_MAX (8U)

static bool g_tx_pending;

static bool wait_inak(bool expected, uint32_t timeout)
{
    while (timeout-- > 0U) {
        const bool inak = ((STM32_CAN1->MSR & CAN_MSR_INAK) != 0U);
        if (inak == expected) { return true; }
    }
    return false;
}

bool mcal_can_init(uint32_t apb1_hz, uint32_t bitrate_hz,
                   uint16_t filter_id, uint32_t timeout)
{
    const uint32_t denominator = bitrate_hz * CAN_SAMPLE_TQ;
    uint32_t prescaler;
    uint32_t btr;
    if ((bitrate_hz == 0U) || (bitrate_hz > (UINT32_MAX / CAN_SAMPLE_TQ)) ||
        (filter_id > CAN_STD_ID_MAX) || (timeout == 0U)) {
        return false;
    }
    if ((apb1_hz == 0U) || ((apb1_hz % denominator) != 0U)) {
        return false;
    }
    prescaler = apb1_hz / denominator;
    if ((prescaler == 0U) || (prescaler > CAN_BRP_MAX)) {
        return false;
    }
    g_tx_pending = false;
    /* Enable AFIO then set remap 10: PB8=RX, PB9=TX. */
    STM32_RCC->APB2ENR |= STM32_RCC_APB2ENR_AFIOEN;
    (void)STM32_RCC->APB2ENR;
    /* SWJ_CFG[26:24] is write-only (undefined on read).
     * Explicitly write 000 to preserve JTAG/SWD while remapping CAN. */
    STM32_AFIO_MAPR =
        (STM32_AFIO_MAPR & ~(STM32_AFIO_MAPR_CAN_REMAP_MASK |
                            STM32_AFIO_MAPR_SWJ_CFG_MASK)) |
        STM32_AFIO_MAPR_CAN_REMAP_PB89;
    STM32_RCC->APB1ENR |= STM32_RCC_APB1ENR_CAN1EN;
    (void)STM32_RCC->APB1ENR;
    STM32_RCC->APB1RSTR |= STM32_RCC_APB1RSTR_CAN1RST;
    STM32_RCC->APB1RSTR &= ~STM32_RCC_APB1RSTR_CAN1RST;
    STM32_CAN1->MCR = CAN_MCR_INRQ | CAN_MCR_NART | CAN_MCR_ABOM;
    if (!wait_inak(true, timeout)) { return false; }
    /* 16 total time quanta; BS1=13, BS2=2, SJW=1; 87.5% sample point. */
    btr = ((CAN_SJW - 1U) << 24U) |
          ((CAN_TS2 - 1U) << 20U) |
          ((CAN_TS1 - 1U) << 16U) |
          (prescaler - 1U);
    STM32_CAN1->BTR = btr;
    /* 32-bit mask filter 0; standard DATA ID only, FIFO0. */
    STM32_CAN1->FMR |= CAN_FMR_FINIT;
    STM32_CAN1->FA1R &= ~CAN_FILTER_BANK0;
    STM32_CAN1->FM1R &= ~CAN_FILTER_BANK0;
    STM32_CAN1->FS1R |= CAN_FILTER_BANK0;
    STM32_CAN1->FFA1R &= ~CAN_FILTER_BANK0;
    STM32_CAN1->FILTER[0].FR1 = ((uint32_t)filter_id << 21U);
    STM32_CAN1->FILTER[0].FR2 = (UINT32_C(0x7FF) << 21U) | CAN_RIR_IDE | CAN_RIR_RTR;
    STM32_CAN1->FA1R |= CAN_FILTER_BANK0;
    STM32_CAN1->FMR &= ~CAN_FMR_FINIT;
    STM32_CAN1->IER = CAN_IER_FMPIE0;
    STM32_CAN1->MCR &= ~CAN_MCR_INRQ;
    return wait_inak(false, timeout);
}

bool mcal_can_transmit(const can_frame_t *frame)
{
    uint32_t low = 0U;
    uint32_t high = 0U;
    uint32_t index;
    if ((frame == NULL) || (frame->id > CAN_STD_ID_MAX) ||
        (frame->dlc > CAN_DLC_MAX) || g_tx_pending ||
        ((STM32_CAN1->TSR & CAN_TSR_TME0) == 0U)) {
        return false;
    }
    for (index = 0U; index < 4U; ++index) {
        low |= ((uint32_t)frame->data[index] << (index * 8U));
        high |= ((uint32_t)frame->data[index + 4U] << (index * 8U));
    }
    STM32_CAN1->TSR = CAN_TSR_RQCP0;
    STM32_CAN1->TX[0].TIR = ((uint32_t)frame->id << 21U);
    STM32_CAN1->TX[0].TDTR = (uint32_t)frame->dlc;
    STM32_CAN1->TX[0].TDLR = low;
    STM32_CAN1->TX[0].TDHR = high;
    g_tx_pending = true;
    STM32_CAN1->TX[0].TIR |= CAN_TIR_TXRQ;
    return true;
}

mcal_can_tx_state_t mcal_can_poll_tx(void)
{
    uint32_t status;
    if (!g_tx_pending) { return MCAL_CAN_TX_IDLE; }
    status = STM32_CAN1->TSR;
    if ((status & CAN_TSR_RQCP0) == 0U) { return MCAL_CAN_TX_PENDING; }
    g_tx_pending = false;
    STM32_CAN1->TSR = CAN_TSR_RQCP0;
    return ((status & CAN_TSR_TXOK0) != 0U)
               ? MCAL_CAN_TX_OK : MCAL_CAN_TX_ERROR;
}

bool mcal_can_pop_fifo0(can_frame_t *frame)
{
    uint32_t identifier;
    uint32_t length;
    uint32_t index;
    uint32_t low;
    uint32_t high;
    if ((frame == NULL) || ((STM32_CAN1->RF0R & CAN_RF0R_FMP0_MASK) == 0U)) {
        return false;
    }
    identifier = STM32_CAN1->RX[0].RIR;
    length = STM32_CAN1->RX[0].RDTR & UINT32_C(0xF);
    low = STM32_CAN1->RX[0].RDLR;
    high = STM32_CAN1->RX[0].RDHR;
    if (((identifier & (CAN_RIR_IDE | CAN_RIR_RTR)) != 0U) ||
        (length > CAN_DLC_MAX)) {
        STM32_CAN1->RF0R = CAN_RF0R_RFOM0;
        return false;
    }
    frame->id = (uint16_t)((identifier >> 21U) & CAN_STD_ID_MAX);
    frame->dlc = (uint8_t)length;
    for (index = 0U; index < 4U; ++index) {
        frame->data[index] = (uint8_t)(low >> (index * 8U));
        frame->data[index + 4U] = (uint8_t)(high >> (index * 8U));
    }
    STM32_CAN1->RF0R = CAN_RF0R_RFOM0;
    return true;
}

uint32_t mcal_can_get_esr(void)
{
    return STM32_CAN1->ESR;
}
