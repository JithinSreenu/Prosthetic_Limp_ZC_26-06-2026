/**
 * @file    uart_dma.h
 * @brief   UART DMA Communication Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Provides non-blocking UART communication using DMA.
 * 
 * DMA UART ADVANTAGES:
 * Traditional polling UART wastes CPU cycles:
 * 
 * POLLING:
 * CPU: "Is data ready? No. Is data ready? No. Is data ready? Yes!"
 * Result: 99% CPU time wasted waiting
 * 
 * DMA:
 * CPU: "Start transfer. Tell me when done."
 * DMA: *transfers data in background*
 * DMA: "Transfer complete!"
 * Result: CPU free for real work
 * 
 * INTERRUPT vs DMA:
 * Interrupt UART: CPU interrupted for EVERY byte
 * DMA UART: CPU interrupted only when entire buffer complete
 * For 100 bytes: 100 interrupts vs 1 interrupt = 99% reduction
 * ============================================================================
 */

#ifndef UART_DMA_H
#define UART_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

/** Maximum packet size for reception buffer */
#define UART_RX_BUFFER_SIZE     (64U)

/** Maximum packet size for transmission */
#define UART_TX_BUFFER_SIZE     (128U)

/** UART instance identifiers */
typedef enum {
    UART_INSTANCE_1 = 0,    /* USART1 - Bluetooth */
    UART_INSTANCE_2 = 1     /* USART2 - DMC */
} UartInstance_t;

/**
 * @brief UART callback event types
 */
typedef enum {
    UART_EVENT_TX_COMPLETE = 0,    /* Transmission finished */
    UART_EVENT_RX_COMPLETE,        /* Reception finished (idle detected) */
    UART_EVENT_RX_HALF,            /* Half buffer received */
    UART_EVENT_ERROR               /* Communication error */
} UartEvent_t;

/**
 * @brief UART receive packet structure
 * 
 * When a complete packet is received (idle line detected),
 * it is stored in this structure and queued for processing.
 */
typedef struct {
    uint8_t data[UART_RX_BUFFER_SIZE];  /* Received data */
    uint16_t length;                     /* Number of bytes received */
    UartInstance_t instance;             /* Which UART received it */
} UartRxPacket_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize UART peripheral with DMA
 * 
 * Configures specified UART instance:
 * - GPIO pins for TX/RX
 * - UART parameters (baud, format)
 * - DMA channels for TX and RX
 * - Starts idle-line detection reception
 * 
 * @param instance UART instance to initialize
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef UART_DMA_Init(UartInstance_t instance);

/**
 * @brief Transmit data via DMA
 * 
 * Non-blocking transmission. Function returns immediately,
 * data is sent in background by DMA.
 * 
 * @param instance UART instance to use
 * @param data Pointer to data buffer
 * @param length Number of bytes to send
 * @return HAL_StatusTypeDef HAL_OK if transfer started, 
 *         HAL_BUSY if previous transfer not complete
 */
HAL_StatusTypeDef UART_DMA_Transmit(UartInstance_t instance, 
                                     const uint8_t *data, 
                                     uint16_t length);

/**
 * @brief Check if transmission is complete
 * 
 * @param instance UART instance to check
 * @return bool true if TX DMA is idle
 */
bool UART_DMA_IsTxComplete(UartInstance_t instance);

/**
 * @brief Get received packet from queue
 * 
 * Retrieves next received packet from internal queue.
 * Non-blocking - returns immediately if no packet available.
 * 
 * @param packet Pointer to store received packet
 * @return bool true if packet available, false if queue empty
 */
bool UART_DMA_GetRxPacket(UartRxPacket_t *packet);

/**
 * @brief Register callback for UART events
 * 
 * Callback is called from DMA interrupt context.
 * Keep processing minimal - use queues for data.
 * 
 * @param instance UART instance
 * @param event Event type to register for
 * @param callback Function pointer to call
 * @param user_data User data passed to callback
 */
void UART_DMA_RegisterCallback(UartInstance_t instance,
                                UartEvent_t event,
                                void (*callback)(void *user_data),
                                void *user_data);

/**
 * @brief Get UART handle for direct HAL access
 * 
 * Provides access to underlying HAL handle for
 * advanced operations not covered by this API.
 * 
 * @param instance UART instance
 * @return UART_HandleTypeDef* Pointer to HAL handle
 */
UART_HandleTypeDef* UART_DMA_GetHandle(UartInstance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* UART_DMA_H */
