/**
 * @file    state_machine.c
 * @brief   Gait State Machine Implementation
 */

#include "state_machine.h"
#include "app_config.h"

static StateMachineContext_t sm_context = {0};
static StateMachineConfig_t sm_config = {
    .force_standing_threshold = 50,      /* 50N to consider standing */
    .force_walking_threshold = 20,       /* 20N variation for walking */
    .angle_change_threshold = 5,         /* 5 degrees change */
    .angle_sitting_threshold = 90,       /* >90 deg flexion = sitting */
    .stable_time_ms = 500,               /* 500ms stable = standing */
    .walking_timeout_ms = 5000           /* 5s without change = idle */
};

void StateMachine_Init(void)
{
    sm_context.current_state = STATE_IDLE;
    sm_context.previous_state = STATE_IDLE;
    sm_context.last_event = EVENT_NONE;
    sm_context.state_entry_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    sm_context.transition_count = 0;
}

GaitState_t StateMachine_Update(const ADC_FilteredData_t *sensor_data)
{
    if (sensor_data == NULL) return sm_context.current_state;
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t time_in_state = current_time - sm_context.state_entry_time;
    
    StateEvent_t event = EVENT_NONE;
    
    /* Detect force high */
    if (sensor_data->force > sm_config.force_standing_threshold) {
        event = EVENT_FORCE_HIGH;
    } else {
        event = EVENT_FORCE_LOW;
    }
    
    /* Detect angle change */
    int16_t angle_diff = abs(sensor_data->knee_angle - (int16_t)sm_context.last_angle);
    if (angle_diff > sm_config.angle_change_threshold) {
        event = EVENT_ANGLE_CHANGING;
        sm_context.angle_change_timer = current_time;
        sm_context.angle_is_changing = true;
    } else if ((current_time - sm_context.angle_change_timer) > sm_config.stable_time_ms) {
        sm_context.angle_is_changing = false;
    }
    
    sm_context.last_angle = sensor_data->knee_angle;
    
    /* --- STATE TRANSITION LOGIC --- */
    switch (sm_context.current_state)
    {
        case STATE_IDLE:
            if (event == EVENT_FORCE_HIGH) {
                sm_context.current_state = STATE_STANDING;
            }
            break;
            
        case STATE_STANDING:
            if (event == EVENT_FORCE_LOW) {
                sm_context.current_state = STATE_IDLE;
            } else if (sm_context.angle_is_changing) {
                sm_context.current_state = STATE_WALKING;
            }
            break;
            
        case STATE_WALKING:
            if (event == EVENT_FORCE_LOW) {
                sm_context.current_state = STATE_IDLE;
            } else if (!sm_context.angle_is_changing && event == EVENT_FORCE_HIGH) {
                sm_context.current_state = STATE_STANDING;
            }
            break;
            
        case STATE_SITTING:
            if (event == EVENT_FORCE_HIGH) {
                sm_context.current_state = STATE_STANDING;
            }
            break;
            
        case STATE_CALIBRATION:
            /* Stay here until explicitly forced out */
            break;
            
        case STATE_FAULT:
            /* Stay here until explicitly cleared */
            break;
            
        default:
            sm_context.current_state = STATE_IDLE;
            break;
    }
    
    /* Handle state entry actions */
    if (sm_context.current_state != sm_context.previous_state) {
        sm_context.state_entry_time = current_time;
        sm_context.transition_count++;
        sm_context.previous_state = sm_context.current_state;
    }
    
    return sm_context.current_state;
}

void StateMachine_ForceState(GaitState_t new_state)
{
    sm_context.previous_state = sm_context.current_state;
    sm_context.current_state = new_state;
    sm_context.state_entry_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

GaitState_t StateMachine_GetState(void)
{
    return sm_context.current_state;
}

const char* StateMachine_GetStateName(GaitState_t state)
{
    switch (state) {
        case STATE_IDLE: return "IDLE";
        case STATE_STANDING: return "STANDING";
        case STATE_WALKING: return "WALKING";
        case STATE_SITTING: return "SITTING";
        case STATE_STAIR_ASCENT: return "STAIR_ASC";
        case STATE_STAIR_DESCENT: return "STAIR_DESC";
        case STATE_CALIBRATION: return "CALIB";
        case STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

bool StateMachine_IsTransitionValid(GaitState_t target_state) { return true; }
uint32_t StateMachine_GetTimeInState(void) { return 0; }
void StateMachine_Reset(void) { StateMachine_Init(); }
