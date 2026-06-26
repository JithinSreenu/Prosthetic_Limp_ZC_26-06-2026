/**
 * @file    motor_control.c
 * @brief   Motor PWM Control Implementation
 */

#include "motor_control.h"
#include "stm32u5xx_hal.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================
 */
TIM_HandleTypeDef htim1 = {0};
TIM_HandleTypeDef htim2 = {0};

static MotorStatus_t motor_a_status = {0};
static MotorStatus_t motor_b_status = {0};

static MotorConfig_t motor_config = {
    .max_duty = PWM_MAX_DUTY,
    .ramp_rate = RAMP_RATE_PER_MS,
    .stall_timeout_ms = 1000
};

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

HAL_StatusTypeDef Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* --------------------------------------------------------------------
     * MOTOR A PWM: TIM1 CH1 on PA8
     * -------------------------------------------------------------------- */
    __HAL_RCC_TIM1_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = MOTOR_A_PWM_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* PWM needs fast edges */
    GPIO_InitStruct.Alternate = MOTOR_A_PWM_AF;
    HAL_GPIO_Init(MOTOR_A_PWM_PORT, &GPIO_InitStruct);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = TIM_PRESCALER - 1; /* Prescaler value is period-1 */
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = TIM_ARR_VALUE; /* 999 for 10kHz */
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0; /* For basic PWM, no repetition counting needed */
if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) return HAL_ERROR;
/* Configure PWM Channel 1 for Motor A /
sConfigOC.OCMode = TIM_OCMODE_PWM1; / Output high when counter < CCR /
sConfigOC.Pulse = 0; / Initial duty cycle = 0% */
sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, MOTOR_A_PWM_CHANNEL) != HAL_OK) return HAL_ERROR;
/* --------------------------------------------------------------------
MOTOR B PWM: TIM2 CH1 on PA15
-------------------------------------------------------------------- */
__HAL_RCC_TIM2_CLK_ENABLE();
GPIO_InitStruct.Pin = MOTOR_B_PWM_PIN;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = MOTOR_B_PWM_AF;
HAL_GPIO_Init(MOTOR_B_PWM_PORT, &GPIO_InitStruct);
htim2.Instance = TIM2;
htim2.Init.Prescaler = TIM_PRESCALER - 1;
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = TIM_ARR_VALUE;
htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) return HAL_ERROR;
if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, MOTOR_B_PWM_CHANNEL) != HAL_OK) return HAL_ERROR;
/* Start PWM generation */
HAL_TIM_PWM_Start(&htim1, MOTOR_A_PWM_CHANNEL);
HAL_TIM_PWM_Start(&htim2, MOTOR_B_PWM_CHANNEL);
/* Initialize status structures */
motor_a_status.target_duty = 0;
motor_a_status.current_duty = 0;
motor_a_status.direction = MOTOR_DIR_STOP;
motor_a_status.is_running = false;
motor_b_status.target_duty = 0;
motor_b_status.current_duty = 0;
motor_b_status.direction = MOTOR_DIR_STOP;
motor_b_status.is_running = false;
return HAL_OK;
}

/**

@brief Set motor duty cycle with target (ramping handled in update)
*/
HAL_StatusTypeDef Motor_SetDuty(MotorId_t motor, uint16_t duty, MotorDirection_t direction)
{
MotorStatus_t *status = (motor == MOTOR_A) ? &motor_a_status : &motor_b_status;
/* Clamp duty to valid range */
if (duty > motor_config.max_duty) duty = motor_config.max_duty;
status->target_duty = duty;
status->direction = direction;
status->is_running = (duty > 0 && direction != MOTOR_DIR_STOP);
/* Update direction GPIO immediately */
GPIO_PinState dir_state = (direction == MOTOR_DIR_FORWARD) ? GPIO_PIN_RESET : GPIO_PIN_SET;
if (motor == MOTOR_A) {
HAL_GPIO_WritePin(MOTOR_A_DIR_PORT, MOTOR_A_DIR_PIN, dir_state);
} else {
HAL_GPIO_WritePin(MOTOR_B_DIR_PORT, MOTOR_B_DIR_PIN, dir_state);
}
return HAL_OK;
}

/**

@brief Emergency stop single motor
*/
HAL_StatusTypeDef Motor_EmergencyStop(MotorId_t motor)
{
MotorStatus_t *status = (motor == MOTOR_A) ? &motor_a_status : &motor_b_status;
TIM_HandleTypeDef *htim = (motor == MOTOR_A) ? &htim1 : &htim2;
uint32_t channel = (motor == MOTOR_A) ? MOTOR_A_PWM_CHANNEL : MOTOR_B_PWM_CHANNEL;
/* Immediately set duty to 0 in hardware */
__HAL_TIM_SET_COMPARE(htim, channel, 0);
/* Update software state */
status->target_duty = 0;
status->current_duty = 0;
status->is_running = false;
status->direction = MOTOR_DIR_STOP;
return HAL_OK;
}

/**

@brief Emergency stop ALL motors (Called by Safety Module)
*/
void Motor_EmergencyStopAll(void)
{
Motor_EmergencyStop(MOTOR_A);
Motor_EmergencyStop(MOTOR_B);
}
/**

@brief Update motor ramping (Call every 1ms from Motor Task)
/
void Motor_UpdateRamp(void)
{
/ Process Motor A */
if (motor_a_status.current_duty != motor_a_status.target_duty)
{
if (motor_a_status.current_duty < motor_a_status.target_duty) {
motor_a_status.current_duty += motor_config.ramp_rate;
if (motor_a_status.current_duty > motor_a_status.target_duty)
motor_a_status.current_duty = motor_a_status.target_duty;
} else {
motor_a_status.current_duty -= motor_config.ramp_rate;
if (motor_a_status.current_duty < motor_a_status.target_duty)
motor_a_status.current_duty = motor_a_status.target_duty;
}
__HAL_TIM_SET_COMPARE(&htim1, MOTOR_A_PWM_CHANNEL, motor_a_status.current_duty);
}
/* Process Motor B */
if (motor_b_status.current_duty != motor_b_status.target_duty)
{
if (motor_b_status.current_duty < motor_b_status.target_duty) {
motor_b_status.current_duty += motor_config.ramp_rate;
if (motor_b_status.current_duty > motor_b_status.target_duty)
motor_b_status.current_duty = motor_b_status.target_duty;
} else {
motor_b_status.current_duty -= motor_config.ramp_rate;
if (motor_b_status.current_duty < motor_b_status.target_duty)
motor_b_status.current_duty = motor_b_status.target_duty;
}
__HAL_TIM_SET_COMPARE(&htim2, MOTOR_B_PWM_CHANNEL, motor_b_status.current_duty);
}
}

/**

@brief High-level valve position control (0-100%)
*/
HAL_StatusTypeDef Motor_SetValvePosition(uint8_t position)
{
if (position > 100) position = 100;
/* Map 0-100% to 0-1000 PWM duty */
uint16_t duty = (uint16_t)(position * 10);
if (position > 50) {
/* Open valve /
Motor_SetDuty(MOTOR_A, duty - 500, MOTOR_DIR_FORWARD);
Motor_SetDuty(MOTOR_B, 0, MOTOR_DIR_STOP);
} else {
/ Close valve */
Motor_SetDuty(MOTOR_B, 500 - duty, MOTOR_DIR_FORWARD);
Motor_SetDuty(MOTOR_A, 0, MOTOR_DIR_STOP);
}
return HAL_OK;
}

const MotorStatus_t* Motor_GetStatus(MotorId_t motor)
{
return (motor == MOTOR_A) ? &motor_a_status : &motor_b_status;
}
