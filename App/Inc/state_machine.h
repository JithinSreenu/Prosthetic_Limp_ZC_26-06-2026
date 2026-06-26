/**
 * @file    state_machine.h
 * @brief   Gait State Machine Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Implements finite state machine for gait phase detection.
 * 
 * STATE MACHINE CONCEPT:
 * A finite state machine (FSM) models system behavior as:
 * - A set of STATES (what the system is doing now)
 * - TRANSITIONS (conditions to move between states)
 * - ACTIONS (things that happen in each state)
 * 
 * FOR PROSTHETIC KNEE:
 * States represent phases of movement:
 * IDLE → STANDING → WALKING → STAIR_ASCENT → ...
 * 
 * Transitions are triggered by sensor patterns:
 * - Force > threshold AND Angle changing → WALKING
 * - Force > threshold AND Angle stable → STANDING
 * - Force < threshold → SITTING/IDLE
 * ============================================================================
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "adc_manager.h"

/* ============================================================================
 * STATE TRANSITION STRUCTURE
 * ============================================================================
 */

/**
 * @brief State transition event
 * 
 * Defines the condition that triggers a state change.
 * Multiple events can trigger the same transition (OR logic).
 */
typedef enum {
    EVENT_NONE = 0,
    EVENT_FORCE_HIGH,           /* Force exceeded standing threshold */
    EVENT_FORCE_LOW,            /* Force dropped below threshold */
    EVENT_ANGLE_CHANGING,       /* Knee angle is actively changing */
    EVENT_ANGLE_STABLE,         /* Knee angle is not changing */
    EVENT_STAIR_DETECTED,       /* Stair pattern recognized */
    EVENT_CALIBRATION_REQ,      /* Calibration command received */
    EVENT_FAULT_DETECTED,       /* Safety fault occurred */
    EVENT_FAULT_CLEARED,        /* Fault condition resolved */
    EVENT_TIMEOUT               /* State timeout occurred */
} StateEvent_t;

/**
 * @brief State machine configuration structure
 * 
 * Holds all thresholds and timing for state detection.
 * These values would typically be loaded from Flash/EEPROM
 * and adjustable via Bluetooth commands.
 */
typedef struct {
    /* Force thresholds for state detection (Newtons) */
    int16_t force_standing_threshold;     /* Force to detect standing */
    int16_t force_walking_threshold;      /* Force variation for walking */
    
    /* Angle thresholds (degrees) */
    int16_t angle_change_threshold;       /* Degrees change to detect movement */
    int16_t angle_sitting_threshold;      /* Angle to detect sitting */
    
    /* Timing thresholds (milliseconds) */
    uint32_t stable_time_ms;              /* Time angle must be stable */
    uint32_t walking_timeout_ms;          /* Max time in walking without transition */
    
} StateMachineConfig_t;

/**
 * @brief State machine context structure
 * 
 * Contains all runtime state for the state machine.
 * This includes current state, event history, and timing.
 */
typedef struct {
    GaitState_t current_state;            /* Current gait state */
    GaitState_t previous_state;           /* Previous state (for debug) */
    StateEvent_t last_event;              /* Last event that caused transition */
    uint32_t state_entry_time;            /* Tick count when state was entered */
    uint32_t last_angle;                  /* Previous angle reading */
    uint32_t angle_change_timer;          /* Timer for angle stability */
    bool angle_is_changing;               /* Flag: angle currently changing */
    uint32_t transition_count;            /* Total state transitions (diagnostic) */
} StateMachineContext_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize state machine
 * 
 * Sets initial state to IDLE and clears all timers.
 * 
 * @param None
 */
void StateMachine_Init(void);

/**
 * @brief Update state machine with new sensor data
 * 
 * Call this function periodically (every 5ms) with filtered
 * sensor data. Function evaluates transition conditions
 * and updates state accordingly.
 * 
 * @param sensor_data Pointer to filtered sensor values
 * @return GaitState_t Current state after update
 */
GaitState_t StateMachine_Update(const ADC_FilteredData_t *sensor_data);

/**
 * @brief Force state transition
 * 
 * Forces transition to specified state.
 * Used for calibration mode and fault handling.
 * 
 * @param new_state State to transition to
 */
void StateMachine_ForceState(GaitState_t new_state);

/**
 * @brief Get current state
 * 
 * @return GaitState_t Current gait state
 */
GaitState_t StateMachine_GetState(void);

/**
 * @brief Get state name string
 * 
 * Returns human-readable state name for debug output.
 * 
 * @param state State to get name for
 * @return const char* State name string
 */
const char* StateMachine_GetStateName(GaitState_t state);

/**
 * @brief Check if state transition is valid
 * 
 * Validates that transition from current to target state is allowed.
 * Prevents illegal state jumps.
 * 
 * @param target_state Proposed new state
 * @return bool true if transition is valid
 */
bool StateMachine_IsTransitionValid(GaitState_t target_state);

/**
 * @brief Get time spent in current state
 * 
 * @return uint32_t Time in current state (milliseconds)
 */
uint32_t StateMachine_GetTimeInState(void);

/**
 * @brief Reset state machine to IDLE
 * 
 * Clears all history and returns to IDLE state.
 * Used after fault recovery or system reset.
 */
void StateMachine_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* STATE_MACHINE_H */
