/**
 * @file    motor_control.h
 * @brief   Motor Control Module Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Controls hydraulic valve motors via PWM with smooth ramping.
 * 
 * HYDRAULIC SYSTEM OVERVIEW:
 * The prosthetic knee uses hydraulic damping to control movement.
 * Two electric motors operate valves that control fluid flow:
 * 
 * - Motor A (Open Valve): Allows fluid flow → knee can flex
 * - Motor B (Close Valve): Restricts fluid flow → knee resists flexion
 * 
 * By controlling how open/closed the valve is, we control:
 * - Swing phase speed (how fast knee swings forward)
 * - Stance phase support (how much the knee resists buckling)
 * 
 * PWM CONTROL:
 * Motor speed is proportional to PWM duty cycle.
 * 0% = valve stationary
 * 100% = valve moving at maximum speed
 * 
 * RAMPING:
 * Instant duty cycle changes cause hydraulic pressure spikes.
 * Smooth ramping prevents:
 * - Patient discomfort from jerky motion
 * - Mechanical stress on valve components
 * - Unstable control loop behavior
 * ============================================================================
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================
 */

/**
 * @brief Motor identifier
 */
typedef enum {
    MOTOR_A = 0,    /* Open valve motor */
    MOTOR_B = 1     /* Close valve motor */
} MotorId_t;

/**
 * @brief Motor direction
 */
typedef enum {
    MOTOR_DIR_FORWARD = 0,   /* Open direction */
    MOTOR_DIR_BACKWARD = 1,  /* Close direction */
    MOTOR_DIR_STOP = 2       /* Brake/stop */
} MotorDirection_t;

/**
 * @brief Motor status structure
 * 
 * Contains current state of a single motor.
 */
typedef struct {
    uint16_t target_duty;        /* Requested duty cycle (0-1000) */
    uint16_t current_duty;       /* Actual duty cycle after ramping */
    MotorDirection_t direction;  /* Current direction */
    bool is_running;             /* Motor actively moving */
    bool fault_detected;         /* Fault flag */
} MotorStatus_t;

/**
 * @brief Motor control configuration
 */
typedef struct {
    uint16_t max_duty;           /* Maximum allowed duty cycle */
    uint16_t ramp_rate;          /* Duty cycle change per ms */
    uint32_t stall_timeout_ms;   /* Time before declaring stall */
} MotorConfig_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

/**
 * @brief Initialize motor control module
 * 
 * Configures:
 * - Timer peripherals for PWM generation
 * - GPIO pins for PWM output and direction control
 * - Motor status structures
 * 
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Motor_Init(void);

/**
 * @brief Set motor duty cycle with automatic ramping
 * 
 * Sets target duty cycle. Actual duty cycle ramps
 * toward target at configured rate.
 * 
 * @param motor Motor identifier (MOTOR_A or MOTOR_B)
 * @param duty Target duty cycle (0-1000, where 1000 = 100%)
 * @param direction Motor direction
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Motor_SetDuty(MotorId_t motor, uint16_t duty, MotorDirection_t direction);

/**
 * @brief Stop motor immediately (no ramping)
 * 
 * Used for emergency stops and fault conditions.
 * 
 * @param motor Motor identifier
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Motor_EmergencyStop(MotorId_t motor);

/**
 * @brief Stop all motors immediately
 * 
 * Emergency function - stops both motors without ramping.
 * Called by safety module on fault detection.
 */
void Motor_EmergencyStopAll(void);

/**
 * @brief Update motor ramping
 * 
 * Call periodically (every 1ms) to advance duty cycle
 * toward target. Implements smooth ramp algorithm.
 * 
 * @param None
 */
void Motor_UpdateRamp(void);

/**
 * @brief Get motor status
 * 
 * @param motor Motor identifier
 * @return const MotorStatus_t* Pointer to motor status (read-only)
 */
const MotorStatus_t* Motor_GetStatus(MotorId_t motor);

/**
 * @brief Set valve position
 * 
 * Higher-level function that controls both motors
 * to achieve desired valve position (0-100%).
 * 
 * @param position Desired valve position (0=closed, 100=open)
 * @return HAL_StatusTypeDef HAL_OK on success
 */
HAL_StatusTypeDef Motor_SetValvePosition(uint8_t position);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROL_H */
