/**
 * @file    protocol.c
 * @brief   Packet Encoding/Decoding with Big-Endian Conversion & Checksum
 * 
 * DATA OPTIMIZATION EXPLANATION:
 * ARM Cortex-M is LITTLE-ENDIAN by default. This means the least significant
 * byte is stored at the lowest memory address.
 * Example: int16_t value 0x1234 is stored in RAM as [0x34, 0x12].
 * 
 * However, network protocols and Bluetooth standards typically use BIG-ENDIAN
 * (MSB first). To send 0x1234 over the air, we must transmit [0x12, 0x34].
 * 
 * This file manually shifts bytes to enforce Big-Endian transmission,
 * ensuring the receiving GUI/PC parses the data correctly regardless of 
 * the host machine's architecture.
 */

#include "protocol.h"

void Protocol_Init(void)
{
    /* Nothing to initialize for stateless protocol functions */
}

uint16_t Protocol_EncodeTelemetry(const ADC_FilteredData_t *sensor_data,
                                   GaitState_t state, uint8_t *packet)
{
    if (sensor_data == NULL || packet == NULL) return 0;
    
    /* Pointer arithmetic for direct memory mapping */
    /* Layout: [START][LEN][PAYLOAD 10 bytes][CHECKSUM] */
    
    packet[0] = PROTOCOL_START_BYTE;                 /* 0xAA */
    packet[1] = PROTOCOL_PAYLOAD_LEN;                 /* 10   */
    
    /* --- PAYLOAD ENCODING (Big-Endian) --- */
    
    /* Byte 2: Spool Angle (uint8_t) - No endian conversion needed */
    packet[2] = sensor_data->spool_angle;
    
    /* Bytes 3-4: Force (int16_t ±200) 
     * High byte first: (value >> 8) & 0xFF
     * Low byte second: value & 0xFF */
    packet[3] = (uint8_t)((sensor_data->force >> 8) & 0xFF);
    packet[4] = (uint8_t)(sensor_data->force & 0xFF);
    
    /* Bytes 5-6: Knee Angle (int16_t ±200) */
    packet[5] = (uint8_t)((sensor_data->knee_angle >> 8) & 0xFF);
    packet[6] = (uint8_t)(sensor_data->knee_angle & 0xFF);
    
    /* Bytes 7-8: Moment (int16_t ±3000) */
    packet[7] = (uint8_t)((sensor_data->moment >> 8) & 0xFF);
    packet[8] = (uint8_t)(sensor_data->moment & 0xFF);
    
    /* Byte 9: State ID (uint8_t) */
    packet[9] = (uint8_t)state;
    
    /* Bytes 10-11: Battery mV (uint16_t 0-5000) */
    packet[10] = (uint8_t)((sensor_data->battery_mv >> 8) & 0xFF);
    packet[11] = (uint8_t)(sensor_data->battery_mv & 0xFF);
    
    /* Byte 12: Checksum */
    /* Calculate over LEN + PAYLOAD (bytes 1 to 11) */
    packet[12] = Protocol_CalculateChecksum(&packet[1], PROTOCOL_PAYLOAD_LEN + 1);
    
    return PROTOCOL_PACKET_LEN; /* 13 bytes total */
}

bool Protocol_DecodeCommand(const uint8_t *data, uint16_t length, ProtocolCommand_t *command)
{
    if (data == NULL || command == NULL || length < 4) return false;
    
    /* Find start byte */
    int16_t start_idx = Protocol_FindStartByte(data, length);
    if (start_idx < 0) return false;
    
    /* Check minimum length */
    if ((uint16_t)(length - start_idx) < 4) return false;
    
    /* Verify checksum */
    if (!Protocol_VerifyChecksum(&data[start_idx], length - start_idx)) return false;
    
    /* Extract command (assuming it's the first byte of payload) */
    *command = (ProtocolCommand_t)data[start_idx + 2];
    
    return true;
}

uint8_t Protocol_CalculateChecksum(const uint8_t *data, uint16_t length)
{
    uint32_t sum = 0;
    
    for (uint16_t i = 0; i < length; i++)
    {
        sum += data[i];
    }
    
    /* Return lower 8 bits, ignoring overflow */
    return (uint8_t)(sum & 0xFF);
}

bool Protocol_VerifyChecksum(const uint8_t *packet, uint16_t length)
{
    if (packet == NULL || length < 4) return false;
    
    uint8_t received_checksum = packet[length - 1];
    uint8_t calculated_checksum = Protocol_CalculateChecksum(&packet[1], length - 2);
    
    return (received_checksum == calculated_checksum);
}

int16_t Protocol_FindStartByte(const uint8_t *buffer, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        if (buffer[i] == PROTOCOL_START_BYTE) return (int16_t)i;
    }
    return -1;
}
