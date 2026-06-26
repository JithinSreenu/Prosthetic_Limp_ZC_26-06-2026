/**
 * @file    safety.c
 * @brief   Safety Monitoring Implementation
 */

#include "safety.h"
#include "motor_control.h"
#include "state_machine.h"
#include "bluetooth.h"

static SafetyStatus_t safety_status = {0};
static SafetyConfig_t safety_config = {
    .comm_timeout_ms = COMM_TIMEOUT_MS,
    .watchdog_timeout_ms = SAFETY_WATCHDOG_MS,
    .battery_warning_mv = BATTERY_WARNING_MV,
    .battery_critical_mv = BATTERY_MIN_VALID_MV
};

void Safety_Init(void)
{
    safety_status.fault_active = false;
    safety_status.last_error = ERROR_NONE;
    safety_status.fault_count = 0;
    safety_status.comm_timeout_counter = 0;
    safety_status.watchdog_counter = 0;
    safety_status.adc_valid = true;
    safety_status.battery_ok = true;
    safety_status.comm_ok = true;
}

void Safety_Update(const ADC_FilteredData_t *sensor_data)
{
    /* 1. Check Battery Voltage */
    if (sensor_data != NULL)
    {
        if (sensor_data->battery_mv < safety_config.battery_critical_mv)
        {
            Safety_TriggerFault(ERROR_BATTERY_CRITICAL);
            return; /* Exit immediately on fault */
        }
        safety_status.battery_ok = (sensor_data->battery_mv >= safety_config.battery_warning_mv);
    }
    
    /* 2. Check Communication Timeout */
    if (safety_status.comm_timeout_counter >= safety_config.comm_timeout_ms)
    {
        safety_status.comm_ok = false;
        /* Note: We might not trigger full fault for DMC comm loss, just set flag */
        /* Safety_TriggerFault(ERROR_COMM_TIMEOUT); */
    }
    
    /* 3. Check Task Watchdog */
    if (safety_status.watchdog_counter >= safety_config.watchdog_timeout_ms)
    {
        Safety_TriggerFault(ERROR_WATCHDOG_RESET);
        return;
    }
    
    /* Increment counters */
    safety_status.comm_timeout_counter++;
    safety_status.watchdog_counter++;
}

void Safety_TriggerFault(ErrorCode_t error_code)
{
    /* 
     * CRITICAL SECTION:
     * This can be called from ISR or Task context.
     * We disable interrupts briefly to ensure atomic state update.
     */
    taskENTER_CRITICAL();
    
    if (!safety_status.fault_active)
    {
        safety_status.fault_active = true;
        safety_status.last_error = error_code;
        safety_status.fault_count++;
        
        /* Log to history */
        safety_status.error_history[safety_status.error_history_index] = error_code;
        safety_status.error_history_index = (safety_status.error_history_index + 1) % 8;
    }
    
    taskEXIT_CRITICAL();
    
    /* Execute Safe Mode Actions (Outside critical section) */
    Motor_EmergencyStopAll();
    StateMachine_ForceState(STATE_FAULT);
    Bluetooth_SendError(error_code);
}

bool Safety_ClearFault(void)
{
    /* Only clear if no critical conditions remain */
    if (safety_status.battery_ok && safety_status.comm_ok)
    {
        safety_status.fault_active = false;
        safety_status.last_error = ERROR_NONE;
        safety_status.watchdog_counter = 0;
        return true;
    }
    return false;
}

void Safety_RefreshCommWatchdog(void)
{
    safety_status.comm_timeout_counter = 0;
    safety_status.comm_ok = true;
}

void Safety_RefreshTaskWatchdog(void)
{
    safety_status.watchdog_counter = 0;
}

const SafetyStatus_t* Safety_GetStatus(void)
{
    return &safety_status;
}

bool Safety_IsFaultActive(void)
{
    return safety_status.fault_active;
}

void Safety_EmergencyStop(void)
{
    Motor_EmergencyStopAll();
}
