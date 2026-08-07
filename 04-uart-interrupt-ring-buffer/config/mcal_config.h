#ifndef MCAL_CONFIG_H
#define MCAL_CONFIG_H

#define MCAL_RCC_READY_TIMEOUT_CYCLES (1000000UL)

#define MCAL_USART_RX_BUFFER_SIZE (128UL)
#define MCAL_USART_TX_BUFFER_SIZE (128UL)
#define MCAL_USART1_IRQ_PRIORITY  (2UL)

#if MCAL_USART_RX_BUFFER_SIZE < 2UL
#error "MCAL_USART_RX_BUFFER_SIZE must be at least 2."
#endif

#if (MCAL_USART_RX_BUFFER_SIZE & (MCAL_USART_RX_BUFFER_SIZE - 1UL)) != 0UL
#error "MCAL_USART_RX_BUFFER_SIZE must be a power of two."
#endif

#if MCAL_USART_RX_BUFFER_SIZE > 32768UL
#error "MCAL_USART_RX_BUFFER_SIZE must fit the 16-bit ring index."
#endif

#if MCAL_USART_TX_BUFFER_SIZE < 2UL
#error "MCAL_USART_TX_BUFFER_SIZE must be at least 2."
#endif

#if (MCAL_USART_TX_BUFFER_SIZE & (MCAL_USART_TX_BUFFER_SIZE - 1UL)) != 0UL
#error "MCAL_USART_TX_BUFFER_SIZE must be a power of two."
#endif

#if MCAL_USART_TX_BUFFER_SIZE > 32768UL
#error "MCAL_USART_TX_BUFFER_SIZE must fit the 16-bit ring index."
#endif

#if MCAL_USART1_IRQ_PRIORITY > 15UL
#error "MCAL_USART1_IRQ_PRIORITY must be in the range 0..15."
#endif

#endif
