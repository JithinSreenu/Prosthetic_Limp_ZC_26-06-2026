/**
 * @file    adc_manager.h
 * @brief   ADC Manager Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Provides interface for ADC initialization, DMA configuration,
 * and filtered sensor data access.
 * 
 * ARCHITECTURE:
 * The ADC manager handles all analog-to-digital conversion operations.
 * It configures ADC1 for continuous scan mode with DMA circular buffer.
 * 
 * DATA FLOW:
 * Physical Sensors → Signal Conditioning → ADC1 → GPDMA1 → SRAM Buffer
 *                                                         ↓
 *                                                  Filter Module
 *                                                         ↓
 *                                                  Normalized Values
 * ============================================================================
 */

#ifndef ADC_MANAGER_H
#define ADC_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================
 */

/**
 * @brief Raw ADC data structure
 * 
 * This structure holds the raw 16-bit values from the ADC.
 * Even though ADC is 12-bit (0-4095), the register is 16-bit.
 * Upper 4 bits may contain overflow or padding.
 * 
 * MEMORY LAYOUT:
 * Total size: 8 bytes (4 × int16_t)
 * Aligned for DMA access (no padding issues).
 */
typedef struct {
    int16_t load_cell_raw;      /* Raw ADC value from load cell (CH5) */
    int16_t knee_angle_raw;     /* Raw ADC value from angle sensor (CH6) */
    int16_t moment_raw;         /* Raw ADC value from moment sensor (CH7) */
    int16_t battery_raw;        /* Raw ADC value from battery monitor (CH8) */
} ADC_RawData_t;

/**
 * @brief Filtered and normalized sensor data structure
 * 
 * This structure contains the processed sensor values in physical units.
 * Values are scaled according to app_config.h constants.
 * 
 * OPTIMIZED DATA TYPES:
 * - int16_t for ±200 range: Uses 2 bytes instead of 4 (float)
 * - int16_t for ±3000 range: Fits in 16-bit signed (-32768 to +32767)
 * - uint16_t for battery mV: 0-5000 fits in unsigned 16-bit
 * 
 * MEMORY SAVINGS:
 * This structure uses 10 bytes total.
 * Equivalent float version would use 24 bytes.
 * 58% memory reduction for telemetry transmission.
 */
typedef struct {
    int16_t force;              /* Normalized force in Newtons (±200) */
    int16_t knee_angle;         /* Normalized knee angle in degrees (±200) */
    int16_t moment;             /* Normalized moment in Nm (±3000) */
    uint16_t battery_mv;        /* Battery voltage in millivolts (0-5000) */
    uint8_t spool_angle;        /* Valve spool position (0-100) */
    uint8_t state_id;           /* Current gait state (0-16) */
} ADC_FilteredData_t;

/**
 * @brief ADC calibration data structure
 * 
 * Stores zero-offset and scale factors for each sensor.
 * Calculated during calibration routine.
 * 
 * CALIBRATION CONCEPT:
 * Each sensor has two calibration points:
 * 1. Zero point: Output when physical quantity is zero
 * 2. Scale factor: Conversion from ADC units to physical units
 * 
 * Example for load cell:
 * - Zero offset: ADC reads 2050 when no load applied
 * - Scale: (200 N) / (2048 ADC counts) = 0.0977 N/count
 */
typedef struct {
    int16_t load_cell_zero;     /* ADC value at zero force */
    float load_cell_scale;      /* N per ADC count */
    
    int16_t knee_angle_zero;    /* ADC value at zero angle */
    float knee_angle_scale;     /* Degrees per ADC count */
    
    int16_t moment_zero;        /* ADC value at zero moment */
    float moment_scale;         /* Nm per ADC count */
    
    uint16_t battery_scale;     /* mV per ADC count */
} ADC_Calibration_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize ADC peripheral and DMA
 * 
 * This function performs complete ADC initialization:
 * 1. Enable ADC and DMA clocks
 * 2. Configure GPIO pins for analog mode
 * 3. Configure ADC parameters (resolution, scan mode, etc.)
 * 4. Configure DMA channel for circular mode
 * 5. Start continuous conversion
 * 
 * @param None
 * @return HAL_StatusTypeDef HAL_OK on success, error code on failure
 * 
 * @note Must be called before any ADC_Get* functions
 * @note This function blocks for ~100µs during ADC calibration
 */
HAL_StatusTypeDef ADC_Init(void);

/**
 * @brief Start ADC DMA conversion
 * 
 * Arms the DMA for continuous circular acquisition.
 * After this call, DMA fills buffer automatically.
 * 
 * @param None
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef ADC_StartDMA(void);

/**
 * @brief Get raw ADC values from DMA buffer
 * 
 * Reads the latest values from the DMA circular buffer.
 * This function is safe to call from any task (no race condition)
 * because we use double-buffering technique.
 * 
 * @param[out] raw_data Pointer to structure to receive raw values
 * @return HAL_StatusTypeDef HAL_OK on success
 * 
 * @warning raw_data must not be NULL
 */
HAL_StatusTypeDef ADC_GetRawValue(ADC_RawData_t *raw_data);

/**
 * @brief Get filtered and normalized sensor values
 * 
 * Applies configured filter (IIR/Moving Average) to raw data
 * and converts to physical units using calibration data.
 * 
 * @param[out] filtered_data Pointer to structure to receive filtered values
 * @return HAL_StatusTypeDef HAL_OK on success
 * 
 * @warning filtered_data must not be NULL
 * @note This is the primary interface for other modules
 */
HAL_StatusTypeDef ADC_GetFilteredValue(ADC_FilteredData_t *filtered_data);

/**
 * @brief Perform ADC calibration
 * 
 * Executes calibration routine:
 * 1. Takes multiple samples with no load
 * 2. Calculates zero offsets
 * 3. Applies known loads (if available)
 * 4. Calculates scale factors
 * 
 * @param None
 * @return HAL_StatusTypeDef HAL_OK on success
 * 
 * @note System should be in CALIBRATION state during this
 * @note Takes approximately 2 seconds to complete
 */
HAL_StatusTypeDef ADC_Calibrate(void);

/**
 * @brief Apply selected filter to raw value
 * 
 * Internal function that applies the active filter type
 * (IIR, Moving Average, or Kalman) to a single channel.
 * 
 * @param channel Channel index (0-3)
 * @param raw_value Raw ADC value to filter
 * @return int16_t Filtered value
 */
int16_t ADC_ApplyFilter(uint8_t channel, int16_t raw_value);

/**
 * @brief Check if ADC data is valid
 * 
 * Validates that all ADC readings are within expected ranges.
 * Used by safety module to detect sensor failures.
 * 
 * @param raw_data Pointer to raw data to validate
 * @return bool true if all values valid, false if any out of range
 */
bool ADC_IsDataValid(const ADC_RawData_t *raw_data);

/**
 * @brief Get ADC DMA buffer fullness
 * 
 * Returns how many complete samples are available in DMA buffer.
 * Useful for determining if we have enough data for filtering.
 * 
 * @return uint16_t Number of complete samples available
 */
uint16_t ADC_GetBufferFullness(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_MANAGER_H */
