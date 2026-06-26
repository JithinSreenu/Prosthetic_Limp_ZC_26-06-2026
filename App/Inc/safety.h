/**
 * @file    safety.h
 * @brief   Safety Monitoring Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Implements safety-critical monitoring and fault handling.
 * 
 * SAFETY PHILOSOPHY:
 * In a medical device, safety is PARAMOUNT. A malfunctioning
 * prosthetic knee could cause:
 * - Patient falls and injury
 * - Damage to the prosthesis
 * - Liability for the manufacturer
 * 
 * The safety module operates INDEPENDENTLY of normal control.
 * It has:
 * - Highest task priority
 * - Fastest execution period (1 ms)
 * - Direct access to motor stop functions
 * - No dependencies on other modules
 * 
 * FAIL-SAFE DESIGN:
 * When any fault is detected, the system enters SAFE MODE:
 * 1. Immediately stop all motors (no ramping)
 * 2. Close hydraulic valve (prevent uncontrolled motion)
 * 3. Set FAULT state in state machine
 * 4. Send error notification via Bluetooth
 * 5. Log error code to internal variables
 * 6. Wait for manual reset or watchdog timeout
 * 
 * FAULT DETECTION METHODS:
 * - ADC range checking (sensor short/open circuit)
 * - Communication timeout (loss of DMC connection)
 * - Motor overcurrent (via duty cycle monitoring)
 * - Battery undervoltage (system brownout risk)
 * - Watchdog timeout (software hang detection)
 * ============================================================================
 */

#ifndef SAFETY_H
#define SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "adc_manager.h"

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================
 */

/**
 * @brief Safety status structure
 * 
 * Contains current safety state and fault information.
 */
typedef struct {
    bool fault_active;              /* True if in fault state */
    ErrorCode_t last_error;         /* Most recent error code */
    ErrorCode_t error_history[8];   /* Last 8 errors (ring buffer) */
    uint8_t error_history_index;    /* Current position in history */
    uint32_t fault_count;           /* Total faults since boot */
    uint32_t last_fault_time;       /* Timestamp of last fault */
    uint32_t comm_timeout_counter;  /* Counts ms without communication */
    uint32_t watchdog_counter;      /* Counts ms since last watchdog refresh */
    bool adc_valid;                 /* ADC readings in valid range */
    bool battery_ok;                /* Battery above minimum */
    bool comm_ok;                   /* Communication active */
} SafetyStatus_t;

/**
 * @brief Safety configuration
 */
typedef struct {
    uint32_t comm_timeout_ms;       /* Communication timeout threshold */
    uint32_t watchdog_timeout_ms;   /* Watchdog timeout threshold */
    uint16_t battery_warning_mv;    /* Battery warning level */
    uint16_t battery_critical_mv;   /* Battery critical level */
} SafetyConfig_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize safety module
 */
void Safety_Init(void);

/**
 * @brief Safety monitoring task function
 * 
 * Call this every 1ms from safety task.
 * Checks all safety conditions and triggers fault if needed.
 * 
 * @param sensor_data Pointer to current sensor readings (can be NULL)
 */
void Safety_Update(const ADC_FilteredData_t *sensor_data);

/**
 * @brief Trigger fault condition
 * 
 * Immediately enters safe mode.
 * Can be called from any context (ISR safe).
 * 
 * @param error_code Reason for fault
 */
void Safety_TriggerFault(ErrorCode_t error_code);

/**
 * @brief Clear fault condition
 * 
 * Allows system to exit fault state.
 * Only effective if original fault condition is resolved.
 * 
 * @return bool true if fault cleared, false if condition persists
 */
bool Safety_ClearFault(void);

/**
 * @brief Refresh communication watchdog
 * 
 * Call this whenever valid communication is received.
 * Resets the communication timeout counter.
 */
void Safety_RefreshCommWatchdog(void);

/**
 * @brief Refresh task watchdog
 * 
 * Call this from lower-priority tasks to prove they're alive.
 */
void Safety_RefreshTaskWatchdog(void);

/**
 * @brief Get safety status
 * 
 * @return const SafetyStatus_t* Pointer to safety status (read-only)
 */
const SafetyStatus_t* Safety_GetStatus(void);

/**
 * @brief Check if system is in safe mode
 * 
 * @return bool true if fault active
 */
bool Safety_IsFaultActive(void);

/**
 * @brief Emergency stop all motors
 * 
 * Directly stops motors without going through normal control.
 * Bypasses ramping for immediate effect.
 */
void Safety_EmergencyStop(void);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_H */
