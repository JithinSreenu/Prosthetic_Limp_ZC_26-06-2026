/**
 * @file    filter.h
 * @brief   Signal Filtering Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Provides multiple digital filter implementations for sensor data.
 * 
 * FILTER THEORY:
 * Raw sensor data contains noise from:
 * - Electrical interference (50/60 Hz mains)
 * - Mechanical vibration
 * - ADC quantization noise
 * - Thermal noise in analog electronics
 * 
 * Filters remove unwanted noise while preserving signal of interest.
 * Human motion signals are typically < 20 Hz, so we filter above this.
 * ============================================================================
 */

#ifndef FILTER_H
#define FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"

/* ============================================================================
 * CONFIGURATION STRUCTURES
 * ============================================================================
 */

/**
 * @brief Moving Average Filter configuration
 * 
 * WINDOW SIZE TRADE-OFFS:
 * - Small window (4-8): Fast response, less smoothing
 * - Large window (16-32): Slow response, more smoothing
 * - Too large: Introduces unacceptable lag for real-time control
 * 
 * For prosthetic knee at 200 Hz processing:
 * - Window of 8 = 40ms delay (acceptable for gait)
 * - Window of 16 = 80ms delay (borderline)
 * - Window of 32 = 160ms delay (too slow for walking)
 */
typedef struct {
    uint8_t window_size;        /* Number of samples to average */
    uint8_t index;              /* Current position in circular buffer */
    int16_t buffer[MA_WINDOW_SIZE]; /* Circular buffer for samples */
    uint32_t sum;               /* Running sum for efficiency */
    bool initialized;           /* Flag: buffer has been filled once */
} MovingAvgFilter_t;

/**
 * @brief IIR (Infinite Impulse Response) Filter configuration
 * 
 * IIR FILTER EQUATION:
 * y[n] = α × x[n] + (1 - α) × y[n-1]
 * 
 * Where:
 * - x[n] = Current input sample
 * - y[n] = Current output (filtered value)
 * - y[n-1] = Previous output
 * - α = Smoothing factor (0.0 to 1.0)
 * 
 * ALPHA SELECTION:
 * - α = 1.0: No filtering (output = input)
 * - α = 0.5: Moderate smoothing
 * - α = 0.2: Heavy smoothing (used in this project)
 * - α = 0.0: Output never changes
 * 
 * MEMORY ADVANTAGE:
 * IIR needs only ONE previous value (2 bytes per channel).
 * Moving Average needs N values (2×N bytes per channel).
 * For 4 channels with window=8: IIR saves 56 bytes.
 */
typedef struct {
    float alpha;                /* Smoothing factor (0.0 to 1.0) */
    int16_t previous_output;    /* Previous filtered value */
    bool initialized;           /* Flag: first sample received */
} IIRFilter_t;

/**
 * @brief Kalman Filter configuration (Optional - for future enhancement)
 * 
 * KALMAN FILTER CONCEPT:
 * Combines prediction (from physical model) with measurement
 * to produce optimal estimate of true value.
 * 
 * ADVANTAGES OVER SIMPLE FILTERS:
 * - Handles non-stationary noise
 * - Provides estimate uncertainty
 * - Optimal for linear systems with Gaussian noise
 * 
 * DISADVANTAGES:
 * - More complex to tune
 * - More computation per sample
 * - Requires system model
 * 
 * For now, structure is defined but implementation is placeholder.
 */
typedef struct {
    float Q;                    /* Process noise covariance */
    float R;                    /* Measurement noise covariance */
    float P;                    /* Estimation error covariance */
    float K;                    /* Kalman gain */
    float x;                    /* State estimate */
    bool initialized;
} KalmanFilter_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize Moving Average filter
 * 
 * Clears buffer and sets window size.
 * 
 * @param filter Pointer to filter structure
 * @param window_size Number of samples to average (2-32)
 */
void Filter_InitMovingAvg(MovingAvgFilter_t *filter, uint8_t window_size);

/**
 * @brief Apply Moving Average filter to new sample
 * 
 * Adds new sample to buffer, removes oldest, returns average.
 * Uses circular buffer for efficiency (no data shifting).
 * 
 * @param filter Pointer to filter structure
 * @param new_sample New raw ADC value
 * @return int16_t Filtered output value
 */
int16_t Filter_ApplyMovingAvg(MovingAvgFilter_t *filter, int16_t new_sample);

/**
 * @brief Initialize IIR filter
 * 
 * Sets alpha parameter and clears state.
 * 
 * @param filter Pointer to filter structure
 * @param alpha Smoothing factor (0.0 to 1.0)
 */
void Filter_InitIIR(IIRFilter_t *filter, float alpha);

/**
 * @brief Apply IIR filter to new sample
 * 
 * Implements: y[n] = α × x[n] + (1 - α) × y[n-1]
 * 
 * @param filter Pointer to filter structure
 * @param new_sample New raw ADC value
 * @return int16_t Filtered output value
 */
int16_t Filter_ApplyIIR(IIRFilter_t *filter, int16_t new_sample);

/**
 * @brief Initialize Kalman filter
 * 
 * Sets noise parameters and clears state.
 * 
 * @param filter Pointer to filter structure
 * @param process_noise Process noise covariance (Q)
 * @param measurement_noise Measurement noise covariance (R)
 */
void Filter_InitKalman(KalmanFilter_t *filter, float process_noise, float measurement_noise);

/**
 * @brief Apply Kalman filter to new sample
 * 
 * Placeholder implementation - returns input unchanged.
 * Full implementation requires system model.
 * 
 * @param filter Pointer to filter structure
 * @param new_sample New raw ADC value
 * @return int16_t Filtered output value
 */
int16_t Filter_ApplyKalman(KalmanFilter_t *filter, int16_t new_sample);

/**
 * @brief Reset all filters
 * 
 * Clears all filter states. Call after calibration
 * or when switching filter types.
 */
void Filter_ResetAll(void);

#ifdef __cplusplus
}
#endif

#endif /* FILTER_H */
