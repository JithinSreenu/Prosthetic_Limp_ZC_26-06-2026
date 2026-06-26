/**
 * @file    bluetooth.c
 * @brief   Bluetooth Telemetry and Command Interface
 */

#include "bluetooth.h"
#include "uart_dma.h"
#include "freertos_tasks.h"

/* Static TX buffer to hold encoded packet */
static uint8_t bt_tx_buffer[PROTOCOL_PACKET_LEN];

HAL_StatusTypeDef Bluetooth_Init(void)
{
    /* UART1 is already initialized by UART_DMA_Init(UART_INSTANCE_1) in main */
    return HAL_OK;
}

HAL_StatusTypeDef Bluetooth_SendTelemetry(const ADC_FilteredData_t *sensor_data, GaitState_t state)
{
    /* 1. Encode the payload into the static buffer */
    uint16_t packet_size = Protocol_EncodeTelemetry(sensor_data, state, bt_tx_buffer);
    
    /* 2. Transmit via DMA. If busy, drop the packet (telemetry is periodic, missing one is ok) */
    return UART_DMA_Transmit(UART_INSTANCE_1, bt_tx_buffer, packet_size);
}

ProtocolCommand_t Bluetooth_ProcessRx(void)
{
    UartRxPacket_t rx_packet;
    ProtocolCommand_t cmd = CMD_NONE;
    
    /* Check if queue has a packet */
    if (UART_DMA_GetRxPacket(&rx_packet))
    {
        /* Only process if it came from UART1 (Bluetooth) */
        if (rx_packet.instance == UART_INSTANCE_1)
        {
            Protocol_DecodeCommand(rx_packet.data, rx_packet.length, &cmd);
        }
    }
    
    return cmd;
}

HAL_StatusTypeDef Bluetooth_SendStatus(void)
{
    /* Placeholder: Implement status packet structure if needed */
    return HAL_OK;
}

HAL_StatusTypeDef Bluetooth_SendError(ErrorCode_t error_code)
{
    /* Placeholder: Implement error packet structure if needed */
    return HAL_OK;
}

bool Bluetooth_IsConnected(void)
{
    /* 
     * HC-05/BC04 EN pin is often used to detect connection.
     * If not wired, assume connected to avoid blocking telemetry.
     */
    return true;
}
