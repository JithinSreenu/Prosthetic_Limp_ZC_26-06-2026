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
    htim1.Init.RepetitionCounter
