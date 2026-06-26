/**
 * @file    filter.c
 * @brief   Signal Filtering Implementations
 */

#include "filter.h"

/* ============================================================================
 * GLOBAL FILTER INSTANCES
 * ============================================================================
 * Declared extern in header, defined here.
 * 4 channels: 0=Force, 1=Angle, 2=Moment, 3=Battery
 */
MovingAvgFilter_t ma_filters[4];
IIRFilter_t iir_filters[4];
KalmanFilter_t kalman_filters[4];

/* ============================================================================
 * MOVING AVERAGE IMPLEMENTATION
 * ============================================================================
 */

void Filter_InitMovingAvg(MovingAvgFilter_t *filter, uint8_t window_size)
{
    if (filter == NULL) return;
    
    filter->window_size = (window_size < 2) ? 2 : window_size;
    filter->index = 0;
    filter->sum = 0;
    filter->initialized = false;
    
    /* Clear buffer */
    for (uint8_t i = 0; i < MA_WINDOW_SIZE; i++)
    {
        filter->buffer[i] = 0;
    }
}

int16_t Filter_ApplyMovingAvg(MovingAvgFilter_t *filter, int16_t new_sample)
{
    if (filter == NULL) return new_sample;
    
    /* 
     * CIRCULAR BUFFER TECHNIQUE:
     * Instead of shifting all elements down by one (slow), we overwrite
     * the oldest element and move the index pointer forward.
     * 
     * [old, old, new] -> [new, old, new] (index moves to next slot)
     */
    
    /* Subtract the oldest value from the running sum */
    filter->sum -= filter->buffer[filter->index];
    
    /* Store new sample and add to sum */
    filter->buffer[filter->index] = new_sample;
    filter->sum += new_sample;
    
    /* Advance index, wrap around at window size */
    filter->index++;
    if (filter->index >= filter->window_size)
    {
        filter->index = 0;
        filter->initialized = true; /* Buffer has been filled once */
    }
    
    /* Calculate average */
    /* If buffer not full yet, divide by actual count to avoid false low readings */
    uint8_t count = filter->initialized ? filter->window_size : filter->index;
    if (count == 0) return new_sample; /* First sample edge case */
    
    return (int16_t)(filter->sum / count);
}

/* ============================================================================
 * IIR FILTER IMPLEMENTATION
 * ============================================================================
 */

void Filter_InitIIR(IIRFilter_t *filter, float alpha)
{
    if (filter == NULL) return;
    
    /* Clamp alpha to valid range */
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    
    filter->alpha = alpha;
    filter->previous_output = 0;
    filter->initialized = false;
}

int16_t Filter_ApplyIIR(IIRFilter_t *filter, int16_t new_sample)
{
    if (filter == NULL) return new_sample;
    
    int16_t output;
    
    if (!filter->initialized)
    {
        /* First sample: initialize output to prevent startup spike */
        output = new_sample;
        filter->initialized = true;
    }
    else
    {
        /* 
         * IIR EQUATION: y[n] = α * x[n] + (1 - α) * y[n-1]
         * 
         * Fixed-point optimization:
         * Multiplying floats is slow on Cortex-M without FPU.
         * We use integer math approximation:
         * y = (α * 256 * x + (256 - α * 256) * y_prev) / 256
         * 
         * For α = 0.20: coeff = 51, inv_coeff = 205
         */
        int32_t coeff = (int32_t)(filter->alpha * 256.0f);
        int32_t inv_coeff = 256 - coeff;
        
        output = (int16_t)((coeff * new_sample + inv_coeff * filter->previous_output) / 256);
    }
    
    filter->previous_output = output;
    return output;
}

/* ============================================================================
 * KALMAN FILTER IMPLEMENTATION (PLACEHOLDER)
 * ============================================================================
 */

void Filter_InitKalman(KalmanFilter_t *filter, float process_noise, float measurement_noise)
{
    if (filter == NULL) return;
    filter->Q = process_noise;
    filter->R = measurement_noise;
    filter->P = 1.0f;
    filter->K = 0.0f;
    filter->x = 0.0f;
    filter->initialized = false;
}

int16_t Filter_ApplyKalman(KalmanFilter_t *filter, int16_t new_sample)
{
    if (filter == NULL) return new_sample;
    /* 
     * Basic 1D Kalman implementation would go here.
     * For now, bypass to IIR equivalent to save code space.
     */
    return new_sample; 
}

void Filter_ResetAll(void)
{
    for (int i = 0; i < 4; i++)
    {
        Filter_InitMovingAvg(&ma_filters[i], MA_WINDOW_SIZE);
        Filter_InitIIR(&iir_filters[i], IIR_ALPHA);
        Filter_InitKalman(&kalman_filters[i], 0.01f, 0.1f);
    }
}
