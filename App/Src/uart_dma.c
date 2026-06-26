/**
 * @file    uart_dma.c
 * @brief   UART DMA Communication Implementation
 * 
 * OFFLINE WORKSTATION NOTE:
 * This manually configures USART1 and USART2, mapping GPIOs to AF7,
 * configuring the GPDMA channels for TX (Normal) and RX (Circular/Idle),
 * and setting up the NVIC for DMA and UART interrupts.
 */

#include "uart_dma.h"
#include "stm32u5xx_hal.h"
#include "freertos_tasks.h" /* For queue handles */

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================
 */

/* HAL UART Handles - manually initialized */
UART_HandleTypeDef huart1 = {0};
UART_HandleTypeDef huart2 = {0};

/* DMA Handles */
DMA_HandleTypeDef hdma_usart1_tx = {0};
DMA_HandleTypeDef hdma_usart1_rx = {0};
DMA_HandleTypeDef hdma_usart2_tx = {0};
DMA_HandleTypeDef hdma_usart2_rx = {0};

/* Reception buffers - must be in non-cached SRAM or cache maintained */
/* For U585, AXI SRAM is cached. To keep it simple and avoid cache maintenance
   issues in this bare-metal setup, we place RX buffers in standard SRAM1 */
__attribute__((section(".sram1"))) static uint8_t uart1_rx_buffer[UART_RX_BUFFER_SIZE];
__attribute__((section(".sram1"))) static uint8_t uart2_rx_buffer[UART_RX_BUFFER_SIZE];

/* Transmission busy flags */
static volatile bool uart1_tx_busy = false;
static volatile bool uart2_tx_busy = false;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================
 */
static void UART_ConfigGPIO(UartInstance_t instance);
static void UART_ConfigDMA(UartInstance_t instance);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================ */

HAL_StatusTypeDef UART_DMA_Init(UartInstance_t instance)
{
    UART_HandleTypeDef *huart = (instance == UART_INSTANCE_1) ? &huart1 : &huart2;
    
    /* 1. Configure GPIOs */
    UART_ConfigGPIO(instance);
    
    /* 2. Configure DMA Channels */
    UART_ConfigDMA(instance);
    
    /* 3. Configure UART Peripheral */
    huart->Init.BaudRate = UART_BAUD_RATE;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.Overrun = UART_OVERRUN_DISABLE;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    
    if (instance == UART_INSTANCE_1) {
        huart->Instance = USART1;
        __HAL_RCC_USART1_CLK_ENABLE();
    } else {
        huart->Instance = USART2;
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    
    if (HAL_UART_Init(huart) != HAL_OK) return HAL_ERROR;
    
    /* 4. Enable UART Interrupts (specifically for Idle Line detection) */
    if (instance == UART_INSTANCE_1) {
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    } else {
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
    
    /* 5. Start Receive-to-Idle DMA */
    /* This is the modern STM32 way to receive variable-length packets.
       It automatically detects when the UART line goes idle and triggers an interrupt. */
    if (instance == UART_INSTANCE_1) {
        return HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_buffer, UART_RX_BUFFER_SIZE);
    } else {
        return HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_rx_buffer, UART_RX_BUFFER_SIZE);
    }
}

HAL_StatusTypeDef UART_DMA_Transmit(UartInstance_t instance, const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) return HAL_ERROR;
    
    bool *busy_flag = (instance == UART_INSTANCE_1) ? &uart1_tx_busy : &uart2_tx_busy;
    UART_HandleTypeDef *huart = (instance == UART_INSTANCE_1) ? &huart1 : &huart2;
    
    /* Wait if previous transmission is still ongoing (Non-blocking check) */
    if (*busy_flag) return HAL_BUSY;
    
    *busy_flag = true;
    
    /* Start DMA transmission. Normal mode means it stops when done. */
    return HAL_UART_Transmit_DMA(huart, (uint8_t*)data, length);
}

bool UART_DMA_IsTxComplete(UartInstance_t instance)
{
    return (instance == UART_INSTANCE_1) ? !uart1_tx_busy : !uart2_tx_busy;
}

bool UART_DMA_GetRxPacket(UartRxPacket_t *packet)
{
    /* 
     * This function is called from tasks. It checks the FreeRTOS queue.
     * The actual queue push happens in the HAL callback (ISR context).
     */
    if (xUartRxQueue == NULL) return false;
    
    /* Wait 0 ticks - immediate return if empty */
    return (xQueueReceive(xUartRxQueue, packet, 0) == pdTRUE);
}

UART_HandleTypeDef* UART_DMA_GetHandle(UartInstance_t instance)
{
    return (instance == UART_INSTANCE_1) ? &huart1 : &huart2;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void UART_ConfigGPIO(UartInstance_t instance)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (instance == UART_INSTANCE_1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = USART1_TX_PIN | USART1_RX_PIN;
        GPIO_InitStruct.Alternate = USART1_AF;
    } else {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        GPIO_InitStruct.Pin = USART2_TX_PIN | USART2_RX_PIN;
        GPIO_InitStruct.Alternate = USART2_AF;
    }
    
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP; /* UART requires pull-ups to prevent noise */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; /* Fast edges for high baud */
    HAL_GPIO_Init((instance == UART_INSTANCE_1) ? USART1_TX_PORT : USART2_TX_PORT, &GPIO_InitStruct);
}

static void UART_ConfigDMA(UartInstance_t instance)
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    if (instance == UART_INSTANCE_1) {
        /* USART1 TX: GPDMA1 CH1 */
        hdma_usart1_tx.Instance = GPDMA1_CHANNEL1;
        hdma_usart1_tx.Init.Request = GPDMA_REQUEST_USART1_TX;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.Permalink = DMA_PERIPHERAL;
        hdma_usart1_tx.Init.BlkHWRequest = DMA_BK_HWREQUEST_SINGLEBURST;
        hdma_usart1_tx.Init.DataAlignment = DMA_DATAALIGN_BYTE;
        hdma_usart1_tx.Init.SrcInc = DMA_MINC_ENABLE;
        hdma_usart1_tx.Init.DestInc = DMA_PINC_DISABLE;
        hdma_usart1_tx.Init.SrcBurstLength = 1;
        hdma_usart1_tx.Init.DestBurstLength = 1;
        hdma_usart1_tx.Init.Priority = DMA_HIGH_PRIORITY;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL; /* Stop after length bytes */
        __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);
        HAL_DMA_Init(&hdma_usart1_tx);
        HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

        /* USART1 RX: GPDMA1 CH2 */
        hdma_usart1_rx.Instance = GPDMA1_CHANNEL2;
        hdma_usart1_rx.Init.Request = GPDMA_REQUEST_USART1_RX;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.Permalink = DMA_PERIPHERAL;
        hdma_usart1_rx.Init.BlkHWRequest = DMA_BK_HWREQUEST_SINGLEBURST;
        hdma_usart1_rx.Init.DataAlignment = DMA_DATAALIGN_BYTE;
        hdma_usart1_rx.Init.SrcInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.DestInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.SrcBurstLength = 1;
        hdma_usart1_rx.Init.DestBurstLength = 1;
        hdma_usart1_rx.Init.Priority = DMA_HIGH_PRIORITY;
        hdma_usart1_rx.Init.Mode = DMA_NORMAL; /* ReceiveToIdle handles re-arming */
        __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);
        HAL_DMA_Init(&hdma_usart1_rx);
        HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
    } else {
        /* USART2 TX: GPDMA1 CH3 */
        hdma_usart2_tx.Instance = GPDMA1_CHANNEL3;
        hdma_usart2_tx.Init.Request = GPDMA_REQUEST_USART2_TX;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.Permalink = DMA_PERIPHERAL;
        hdma_usart2_tx.Init.BlkHWRequest = DMA_BK_HWREQUEST_SINGLEBURST;
        hdma_usart2_tx.Init.DataAlignment = DMA_DATAALIGN_BYTE;
        hdma_usart2_tx.Init.SrcInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.DestInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.SrcBurstLength = 1;
        hdma_usart2_tx.Init.DestBurstLength = 1;
        hdma_usart2_tx.Init.Priority = DMA_MEDIUM_PRIORITY;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
        HAL_DMA_Init(&hdma_usart2_tx);
        HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);

        /* USART2 RX: GPDMA1 CH4 */
        hdma_usart2_rx.Instance = GPDMA1_CHANNEL4;
        hdma_usart2_rx.Init.Request = GPDMA_REQUEST_USART2_RX;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.Permalink = DMA_PERIPHERAL;
        hdma_usart2_rx.Init.BlkHWRequest = DMA_BK_HWREQUEST_SINGLEBURST;
        hdma_usart2_rx.Init.DataAlignment = DMA_DATAALIGN_BYTE;
        hdma_usart2_rx.Init.SrcInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.DestInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.SrcBurstLength = 1;
        hdma_usart2_rx.Init.DestBurstLength = 1;
        hdma_usart2_rx.Init.Priority = DMA_MEDIUM_PRIORITY;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);
        HAL_DMA_Init(&hdma_usart2_rx);
        HAL_NVIC_SetPriority(GPDMA1_Channel4_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel4_IRQn);
    }
}

/* ============================================================================
 * UART DMA CALLBACKS (EXECUTED IN INTERRUPT CONTEXT)
 * ============================================================================ */

/**
 * @brief TX Complete Callback
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uart1_tx_busy = false;
    } else if (huart->Instance == USART2) {
        uart2_tx_busy = false;
    }
}

/**
 * @brief Receive-To-Idle Complete Callback
 * 
 * This is the critical callback for packet reception.
 * It fires when the UART line goes idle (packet finished).
 * 
 * @param huart UART handle
 * @param Size Number of bytes received before idle
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    UartRxPacket_t packet;
    uint8_t *src_buffer = NULL;
    
    if (huart->Instance == USART1 && Size <= UART_RX_BUFFER_SIZE) {
        packet.instance = UART_INSTANCE_1;
        packet.length = Size;
        src_buffer = uart1_rx_buffer;
        /* Re-arm Receive-to-Idle immediately so we don't miss next packet */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_buffer, UART_RX_BUFFER_SIZE);
    } else if (huart->Instance == USART2 && Size <= UART_RX_BUFFER_SIZE) {
        packet.instance = UART_INSTANCE_2;
        packet.length = Size;
        src_buffer = uart2_rx_buffer;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_rx_buffer, UART_RX_BUFFER_SIZE);
    } else {
        return; /* Invalid size or instance */
    }
    
    /* Copy data to packet struct */
    for (uint16_t i = 0; i < Size; i++) {
        packet.data[i] = src_buffer[i];
    }
    
    /* Send to FreeRTOS queue (FromISR version!) */
    if (xUartRxQueue != NULL) {
        xQueueSendFromISR(xUartRxQueue, &packet, pdFALSE);
    }
}

void UART_DMA_RegisterCallback(UartInstance_t instance, UartEvent_t event, 
                                void (*callback)(void *), void *user_data)
{
    /* Placeholder for advanced callback mapping */
}
