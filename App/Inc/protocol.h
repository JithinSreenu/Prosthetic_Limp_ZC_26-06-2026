/**
 * @file    protocol.h
 * @brief   Communication Protocol Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Implements packet encoding/decoding with checksum verification.
 * 
 * PROTOCOL DESIGN PHILOSOPHY:
 * In embedded communication, packets can be corrupted by:
 * - Electrical noise on UART lines
 * - Timing mismatches between devices
 * - Buffer overflows
 * - Software bugs
 * 
 * Robust protocol design includes:
 * 1. Start byte: Synchronize receiver to packet boundary
 * 2. Length field: Enable buffer size validation
 * 3. Checksum: Detect data corruption
 * 4. Structured payload: Enable parsing and validation
 * 
 * CHECKSUM SELECTION:
 * We use simple additive checksum (XOR would also work).
 * 
 * Why not CRC-16/CRC-32?
 * - More computation (important on low-power MCU)
 * - Overkill for short packets over wired connection
 * - Adds 2-4 bytes overhead per packet
 * 
 * Additive checksum catches:
 * - Single bit errors (always)
 * - Multiple bit errors (usually)
 * - Byte swaps (always)
 * 
 * It does NOT catch:
 * - Transposed bytes that sum to same value
 * 
 * For medical device, consider CRC if regulatory requires it.
 * ============================================================================
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "adc_manager.h"

/* ============================================================================
 * PACKET STRUCTURE CONSTANTS
 * ============================================================================
 */

/** Packet format version */
#define PROTOCOL_VERSION        (0x01U)

/**
 * @brief Telemetry packet structure (transmitted to Bluetooth)
 * 
 * MEMORY OPTIMIZATION:
 * This structure is designed for minimal size.
 * Total: 10 bytes payload
 * 
 * BYTE ORDER: Big-Endian (MSB first)
 * Reason: Network standard, easier for debugging with hex viewers
 * 
 * ALIGNMENT: Packed to prevent compiler padding
 * Without __packed, compiler might add padding bytes for alignment.
 * For example, int16_t might start at even address.
 * __packed ensures exact memory layout matches protocol spec.
 */
typedef struct __packed {
    uint8_t spool_angle;        /* 0-100: Valve position (1 byte) */
    int16_t force;              /* ±200 N: Load cell reading (2 bytes) */
    int16_t knee_angle;         /* ±200 deg: Knee angle (2 bytes) */
    int16_t moment;             /* ±3000 Nm: Joint moment (2 bytes) */
    uint8_t state_id;           /* 0-16: Gait state (1 byte) */
    uint16_t battery_mv;        /* 0-5000 mV: Battery voltage (2 bytes) */
} TelemetryPayload_t;

/**
 * @brief Complete transmission packet structure
 * 
 * Layout: [START][LEN][PAYLOAD...][CHECKSUM]
 * Total size: 1 + 1 + 10 + 1 = 13 bytes
 */
typedef struct __packed {
    uint8_t start_byte;                     /* 0xAA: Packet start marker */
    uint8_t length;                         /* 10: Payload length */
    TelemetryPayload_t payload;             /* 10 bytes: Sensor data */
    uint8_t checksum;                       /* Additive checksum */
} TelemetryPacket_t;

/* Verify packet size at compile time */
#define TELEM_PACKET_SIZE_EXPECTED  (13U)
#if sizeof(TelemetryPacket_t) != TELEM_PACKET_SIZE_EXPECTED
    #error "TelemetryPacket_t size mismatch! Check __packed attribute."
#endif

/**
 * @brief Received command packet structure
 */
typedef struct __packed {
    uint8_t start_byte;         /* 0xAA */
    uint8_t length;             /* Payload length */
    uint8_t command;            /* Command code */
    uint8_t data[32];           /* Command data (variable) */
    uint8_t checksum;           /* Checksum */
} CommandPacket_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize protocol module
 */
void Protocol_Init(void);

/**
 * @brief Encode telemetry data into packet
 * 
 * Takes filtered sensor data and creates transmission packet.
 * Handles byte ordering (big-endian) and checksum calculation.
 * 
 * @param sensor_data Pointer to filtered sensor values
 * @param state Current gait state
 * @param[out] packet Pointer to packet buffer to fill
 * @return uint16_t Total packet size in bytes
 */
uint16_t Protocol_EncodeTelemetry(const ADC_FilteredData_t *sensor_data,
                                   GaitState_t state,
                                   uint8_t *packet);

/**
 * @brief Decode received command packet
 * 
 * Validates checksum and extracts command.
 * 
 * @param data Pointer to received data
 * @param length Number of bytes received
 * @param[out] command Pointer to store decoded command
 * @return bool true if packet valid, false if checksum error
 */
bool Protocol_DecodeCommand(const uint8_t *data, 
                             uint16_t length,
                             ProtocolCommand_t *command);

/**
 * @brief Calculate additive checksum
 * 
 * Sums all bytes in buffer, returns lower 8 bits.
 * 
 * @param data Pointer to data
 * @param length Number of bytes
 * @return uint8_t Checksum value
 */
uint8_t Protocol_CalculateChecksum(const uint8_t *data, uint16_t length);

/**
 * @brief Verify packet checksum
 * 
 * @param packet Pointer to complete packet
 * @param length Packet length
 * @return bool true if checksum valid
 */
bool Protocol_VerifyChecksum(const uint8_t *packet, uint16_t length);

/**
 * @brief Find packet start in buffer
 * 
 * Scans buffer for start byte (0xAA).
 * Handles case where garbage bytes precede packet.
 * 
 * @param buffer Buffer to search
 * @param length Buffer length
 * @return int16_t Index of start byte, -1 if not found
 */
int16_t Protocol_FindStartByte(const uint8_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
