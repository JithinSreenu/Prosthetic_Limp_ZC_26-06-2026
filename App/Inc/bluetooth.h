/**
 * @file    bluetooth.h
 * @brief   Bluetooth Communication Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Handles Bluetooth telemetry transmission and command reception.
 * 
 * BLUETOOTH MODULE ASSUMPTIONS:
 * We assume a standard SPP (Serial Port Profile) Bluetooth module
 * such as HC-05, HC-06, or BC04-B.
 * 
 * These modules present a simple UART interface:
 * - No special initialization required
 * - Transparent serial communication
 * - Baud rate must match module configuration
 * 
 * DATA FLOW:
 * 
 * TRANSMIT (Telemetry):
 * Sensor Data → Protocol_EncodeTelemetry() → UART1 DMA → Bluetooth → GUI
 * 
 * RECEIVE (Commands):
 * GUI → Bluetooth → UART1 DMA → Protocol_DecodeCommand() → Command Queue
 * ============================================================================
 */

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "adc_manager.h"
#include "protocol.h"

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize Bluetooth communication
 * 
 * Initializes USART1 with DMA for Bluetooth communication.
 * 
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Bluetooth_Init(void);

/**
 * @brief Send telemetry data
 * 
 * Encodes and transmits sensor data via Bluetooth.
 * Non-blocking - data queued for DMA transmission.
 * 
 * @param sensor_data Pointer to filtered sensor values
 * @param state Current gait state
 * @return HAL_StatusTypeDef HAL_OK if queued, HAL_BUSY if previous TX pending
 */
HAL_StatusTypeDef Bluetooth_SendTelemetry(const ADC_FilteredData_t *sensor_data,
                                           GaitState_t state);

/**
 * @brief Process received Bluetooth data
 * 
 * Checks for received packets and processes commands.
 * Call this periodically from Bluetooth task.
 * 
 * @return ProtocolCommand_t Command received, or CMD_NONE
 */
ProtocolCommand_t Bluetooth_ProcessRx(void);

/**
 * @brief Send status response
 * 
 * Sends current system status in response to GET_STATUS command.
 * 
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Bluetooth_SendStatus(void);

/**
 * @brief Send error notification
 * 
 * Sends fault information to GUI.
 * 
 * @param error_code Error code to send
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Bluetooth_SendError(ErrorCode_t error_code);

/**
 * @brief Check if Bluetooth is connected
 * 
 * Some Bluetooth modules provide a connection status pin.
 * This function checks that status if available.
 * 
 * @return bool true if connected (or status unknown)
 */
bool Bluetooth_IsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUETOOTH_H */
