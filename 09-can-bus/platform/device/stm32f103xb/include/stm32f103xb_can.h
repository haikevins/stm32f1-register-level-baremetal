#ifndef STM32F103XB_CAN_H
#define STM32F103XB_CAN_H
#include <stddef.h>
#include <stdint.h>
#include "stm32f103xb_memory.h"

typedef struct {
    volatile uint32_t TIR;
    volatile uint32_t TDTR;
    volatile uint32_t TDLR;
    volatile uint32_t TDHR;
} stm32_can_tx_mailbox_t;
typedef struct {
    volatile const uint32_t RIR;
    volatile const uint32_t RDTR;
    volatile const uint32_t RDLR;
    volatile const uint32_t RDHR;
} stm32_can_rx_mailbox_t;
typedef struct {
    volatile uint32_t FR1;
    volatile uint32_t FR2;
} stm32_can_filter_bank_t;
typedef struct {
    volatile uint32_t MCR;
    volatile const uint32_t MSR;
    volatile uint32_t TSR;
    volatile uint32_t RF0R;
    volatile uint32_t RF1R;
    volatile uint32_t IER;
    volatile const uint32_t ESR;
    volatile uint32_t BTR;
    uint32_t RESERVED0[88];
    stm32_can_tx_mailbox_t TX[3];
    stm32_can_rx_mailbox_t RX[2];
    uint32_t RESERVED1[12];
    volatile uint32_t FMR;
    volatile uint32_t FM1R;
    uint32_t RESERVED2;
    volatile uint32_t FS1R;
    uint32_t RESERVED3;
    volatile uint32_t FFA1R;
    uint32_t RESERVED4;
    volatile uint32_t FA1R;
    uint32_t RESERVED5[8];
    stm32_can_filter_bank_t FILTER[14];
} stm32_can_registers_t;
_Static_assert(offsetof(stm32_can_registers_t, TX) == 0x180U, "CAN TX offset");
_Static_assert(offsetof(stm32_can_registers_t, RX) == 0x1B0U, "CAN RX offset");
_Static_assert(offsetof(stm32_can_registers_t, FMR) == 0x200U, "CAN filter offset");
_Static_assert(offsetof(stm32_can_registers_t, FILTER) == 0x240U, "CAN filter banks offset");

#define STM32_CAN1_BASE          (STM32_APB1_BASE + UINT32_C(0x6400))
#define STM32_CAN1               ((stm32_can_registers_t *)STM32_CAN1_BASE)
#define STM32_AFIO_MAPR          (*(volatile uint32_t *)(STM32_APB2_BASE + UINT32_C(0x4)))
#define STM32_RCC_APB2ENR_AFIOEN (UINT32_C(1) << 0U)
#define STM32_RCC_APB1ENR_CAN1EN (UINT32_C(1) << 25U)
#define STM32_RCC_APB1RSTR_CAN1RST (UINT32_C(1) << 25U)
#define STM32_AFIO_MAPR_CAN_REMAP_MASK (UINT32_C(3) << 13U)
#define STM32_AFIO_MAPR_CAN_REMAP_PB89 (UINT32_C(2) << 13U)
#define STM32_AFIO_MAPR_SWJ_CFG_MASK (UINT32_C(7) << 24U)

#define CAN_MCR_INRQ (UINT32_C(1) << 0U)
#define CAN_MCR_SLEEP (UINT32_C(1) << 1U)
#define CAN_MCR_NART (UINT32_C(1) << 4U)
#define CAN_MCR_ABOM (UINT32_C(1) << 6U)
#define CAN_MSR_INAK (UINT32_C(1) << 0U)
#define CAN_IER_FMPIE0 (UINT32_C(1) << 1U)
#define CAN_RF0R_FMP0_MASK (UINT32_C(3) << 0U)
#define CAN_RF0R_RFOM0 (UINT32_C(1) << 5U)
#define CAN_TSR_RQCP0 (UINT32_C(1) << 0U)
#define CAN_TSR_TXOK0 (UINT32_C(1) << 1U)
#define CAN_TSR_TME0 (UINT32_C(1) << 26U)
#define CAN_TIR_TXRQ (UINT32_C(1) << 0U)
#define CAN_RIR_RTR (UINT32_C(1) << 1U)
#define CAN_RIR_IDE (UINT32_C(1) << 2U)
#define CAN_FMR_FINIT (UINT32_C(1) << 0U)
#define CAN_FILTER_BANK0 (UINT32_C(1) << 0U)
#endif
