/**
 * @file    main.c
 * @brief   Main Application Entry Point
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * This is the main entry point for the prosthetic knee controller firmware.
 * It initializes all hardware and software components, then starts
 * the FreeRTOS scheduler.
 * 
 * ARCHITECTURE EXPLANATION:
 * In a bare-metal (no OS) system, main() contains an infinite loop
 * that handles everything. With FreeRTOS, main() only does:
 * 
 * 1. Hardware initialization (clocks, GPIO)
 * 2. Peripheral initialization (ADC, UART, timers)
 * 3. RTOS object creation (tasks, queues)
 * 4. Start scheduler (never returns)
 * 
 * After vTaskStartScheduler() is called, the FreeRTOS kernel takes
 * over and runs the highest-priority ready task. The scheduler uses
 * SysTick interrupt to switch between tasks.
 * 
 * BYPASS MECHANISM:
 * We override HAL_InitTick() to prevent HAL from configuring SysTick.
 * FreeRTOS needs exclusive control of SysTick for its scheduler.
 * ============================================================================
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================
 * 
 * Header file inclusion order matters for compilation:
 * 1. Main header (main.h) - includes stdint.h, stdbool.h
 * 2. HAL headers - hardware abstraction layer
 * 3. FreeRTOS headers - real-time operating system
 * 4. Application headers - our custom modules
 */

#include "main.h"
#include "stm32u5xx_hal.h"

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

/* Application module headers */
#include "app_config.h"
#include "adc_manager.h"
#include "filter.h"
#include "state_machine.h"
#include "motor_control.h"
#include "uart_dma.h"
#include "protocol.h"
#include "bluetooth.h"
#include "safety.h"
#include "freertos_tasks.h"

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================
 * 
 * Forward declarations allow functions to be called before their
 * definition appears in the source file. This is required by C.
 */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void Error_Handler(void);

/* ============================================================================
 * BYPASS MECHANISM: HAL_InitTick Override
 * ============================================================================
 * 
 * WHY THIS IS NECESSARY:
 * The HAL library expects to control SysTick for its delay functions
 * (HAL_Delay, etc.). However, FreeRTOS also needs SysTick for its
 * scheduler. Both trying to use SysTick causes conflicts.
 * 
 * SOLUTION:
 * We provide a weak function override that does nothing.
 * This prevents HAL from configuring SysTick, leaving it free
 * for FreeRTOS to use exclusively.
 * 
 * CONSEQUENCE:
 * HAL_Delay() will NOT work. Use vTaskDelay() instead.
 * This is acceptable because we never want blocking delays in RTOS.
 * 
 * The 'weak' attribute in the HAL library allows us to override
 * this function without modifying HAL source code.
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* 
     * Return immediately without initializing SysTick.
     * FreeRTOS will configure SysTick in xPortStartScheduler().
     * 
     * The TickPriority parameter is ignored.
     * We return HAL_OK to indicate "success" even though
     * we did nothing. HAL_Init() checks this return value.
     */
    return HAL_OK;
}

/* ============================================================================
 * MAIN FUNCTION
 * ============================================================================
 */

int main(void)
{
    /* ========================================================================
     * STEP 1: HAL Library Initialization
     * ========================================================================
     * 
     * HAL_Init() performs:
     * - Set NVIC priority grouping to 4 (all preemption, no subpriority)
     *   This is required for FreeRTOS on Cortex-M
     * - Initialize SysTick (but our override makes this a no-op)
     * - Set HAL tick counter to 0
     * 
     * The return value check is defensive programming.
     * HAL_Init() should never fail, but if it does, we go to error handler.
     */
    if (HAL_Init() != HAL_OK)
    {
        Error_Handler();
    }

    /* ========================================================================
     * STEP 2: System Clock Configuration
     * ========================================================================
     * 
     * The STM32U585 boots from internal RC oscillator (MSI) at 4 MHz.
     * We need to switch to external crystal (HSE) and then to PLL
     * for 160 MHz operation.
     * 
     * Clock tree for STM32U585:
     * 
     * HSE (16 MHz) → PLL → SYSCLK (160 MHz)
     *                        ├── AHB (160 MHz) → CPU, DMA, Flash
     *                        ├── APB1 (80 MHz) → USART2, TIM2
     *                        ├── APB2 (80 MHz) → USART1, ADC, TIM1
     *                        └── APB3 (80 MHz) → Other peripherals
     * 
     * WHY 160 MHz?
     * - Fast enough for 1 kHz control loops with margin
     * - Below absolute maximum (250 MHz for STM32U585)
     * - Powers of 2 make timer calculations easier
     */
    SystemClock_Config();

    /* ========================================================================
     * STEP 3: GPIO Initialization
     * ========================================================================
     * 
     * Configure basic GPIO pins that don't belong to specific peripherals.
     * Peripheral-specific GPIO (ADC, UART, TIM) is configured in their
     * respective init functions.
     */
    GPIO_Init();

    /* ========================================================================
     * STEP 4: Peripheral Initialization
     * ========================================================================
     * 
     * Initialize each peripheral in dependency order:
     * 1. ADC (no dependencies)
     * 2. UART1/UART2 (no dependencies)
     * 3. Motors (depends on timers)
     * 4. Safety (depends on ADC being ready)
     * 5. Filters (no dependencies)
     * 6. State Machine (no dependencies)
     * 
     * Each init function handles its own clock and GPIO configuration.
     */
    
    /* Initialize signal filters first (used by ADC module) */
    Filter_InitMovingAvg(&ma_filters[0], MA_WINDOW_SIZE);
    Filter_InitMovingAvg(&ma_filters[1], MA_WINDOW_SIZE);
    Filter_InitMovingAvg(&ma_filters[2], MA_WINDOW_SIZE);
    Filter_InitMovingAvg(&ma_filters[3], MA_WINDOW_SIZE);
    
    Filter_InitIIR(&iir_filters[0], IIR_ALPHA);
    Filter_InitIIR(&iir_filters[1], IIR_ALPHA);
    Filter_InitIIR(&iir_filters[2], IIR_ALPHA);
    Filter_InitIIR(&iir_filters[3], IIR_ALPHA);
    
    /* Initialize ADC with DMA */
    if (ADC_Init() != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Start ADC DMA continuous conversion */
    if (ADC_StartDMA() != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Initialize UART1 for Bluetooth */
    if (UART_DMA_Init(UART_INSTANCE_1) != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Initialize UART2 for DMC communication */
    if (UART_DMA_Init(UART_INSTANCE_2) != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Initialize motor control */
    if (Motor_Init() != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Initialize safety module */
    Safety_Init();
    
    /* Initialize state machine */
    StateMachine_Init();
    
    /* Initialize protocol module */
    Protocol_Init();
    
    /* Initialize Bluetooth module */
    if (Bluetooth_Init() != HAL_OK)
    {
        Error_Handler();
    }

    /* ========================================================================
     * STEP 5: Create FreeRTOS Tasks and Objects
     * ========================================================================
     * 
     * Create all tasks, queues, and synchronization primitives.
     * Tasks are created in suspended state if needed.
     */
    if (Tasks_CreateAll() != pdPASS)
    {
        Error_Handler();
    }

    /* ========================================================================
     * STEP 6: Start FreeRTOS Scheduler
     * ========================================================================
     * 
     * vTaskStartScheduler() does the following:
     * 1. Configures SysTick interrupt for 1 ms tick
     * 2. Creates idle task (lowest priority, runs when nothing else to do)
     * 3. Enables interrupts (if not already enabled)
     * 4. Starts the first task (highest priority ready task)
     * 5. NEVER RETURNS (enters infinite scheduling loop)
     * 
     * If vTaskStartScheduler() returns, it means:
     * - Not enough heap memory for idle task
     * - Timer task creation failed (if timers enabled)
     * 
     * This should never happen with proper heap size configuration.
     */
    vTaskStartScheduler();

    /* ========================================================================
     * STEP 7: Error Handler (Should Never Reach Here)
     * ========================================================================
     * 
     * If we reach this point, the scheduler failed to start.
     * This is a critical error - enter infinite loop.
     */
    while (1)
    {
        /* 
         * In production, this would trigger a visual/audible fault
         * indicator and possibly a system reset.
         */
    }
}

/* ============================================================================
 * SYSTEM CLOCK CONFIGURATION
 * ============================================================================
 * 
 * This function configures the STM32U585 clock tree to achieve
 * 160 MHz system clock from the 16 MHz external crystal.
 * 
 * Normally, STM32CubeMX generates this function. Since we cannot
 * use CubeMX on the offline workstation, we write it manually.
 * 
 * REGISTER-LEVEL EXPLANATION:
 * The RCC (Reset and Clock Control) peripheral manages all clocks.
 * Key registers:
 * - RCC_CR: Clock control (enable HSE, PLL)
 * - RCC_CFGR1/2/3: Clock configuration (PLL multipliers, dividers)
 * - RCC_AHB1/2/3ENR: AHB peripheral clock enables
 * - RCC_APB1/2/3ENR: APB peripheral clock enables
 * 
 * PLL CONFIGURATION FOR 160 MHz:
 * 
 * Input: HSE = 16 MHz
 * 
 * PLL1 (Main PLL):
 *   M = 4  (16/4 = 4 MHz VCO input)
 *   N = 40 (4×40 = 160 MHz VCO output)
 *   P = 2  (160/2 = 80 MHz... wait, that's wrong)
 *   
 * Let me recalculate:
 *   For 160 MHz SYSCLK:
 *   VCO_OUT = HSE/M × N
 *   SYSCLK = VCO_OUT / P
 *   
 *   Target: SYSCLK = 160 MHz
 *   If M=4, N=80: VCO_OUT = 16/4 × 80 = 320 MHz
 *   If P=2: SYSCLK = 320/2 = 160 MHz ✓
 *   
 *   VCO range check: 128-560 MHz for STM32U585
 *   320 MHz is within range ✓
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* --------------------------------------------------------------------
     * STEP 1: Configure oscillator sources
     * --------------------------------------------------------------------
     * 
     * We enable HSE (High Speed External) crystal oscillator.
     * The B-U585I-IOT02A board has a 16 MHz crystal.
     * 
     * OSCILLATOR_TYPE_HSE: Enable HSE
     * HSE_STATE: HSE_BYPASS if crystal is external (like on some boards)
     *            HSE_ON if using on-board crystal (our case)
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    
    /* 
     * Configure PLL1 (Main PLL)
     * 
     * PLLSOURCE_HSE: Use HSE as PLL input (not HSI)
     * PLLM = 4: Divide 16 MHz by 4 = 4 MHz VCO input
     * PLLN = 80: Multiply 4 MHz by 80 = 320 MHz VCO output
     * PLLP = 2: Divide 320 MHz by 2 = 160 MHz system clock
     * 
     * VCO output range: 128-560 MHz
     * 320 MHz is valid ✓
     */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 80;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;   /* 320/4 = 80 MHz for some peripherals */
    RCC_OscInitStruct.PLL.PLLR = 2;   /* 320/2 = 160 MHz (same as P) */
    
    /* Apply oscillator configuration */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* --------------------------------------------------------------------
     * STEP 2: Configure CPU and bus clocks
     * --------------------------------------------------------------------
     * 
     * CLOCK TYPE: Enable all clock domains
     * SYSCLK SOURCE: Use PLL1 output (160 MHz)
     * 
     * AHB PRESCALER: HCLK = SYSCLK / 1 = 160 MHz
     *   AHB bus runs at full speed for CPU and DMA performance
     * 
     * APB1 PRESCALER: PCLK1 = HCLK / 2 = 80 MHz
     *   APB1 peripherals run at 80 MHz
     *   Timer clock = 80 × 2 = 160 MHz (doubled when prescaler > 1)
     * 
     * APB2 PRESCALER: PCLK2 = HCLK / 2 = 80 MHz
     *   APB2 peripherals (ADC, USART1, TIM1) run at 80 MHz
     *   Timer clock = 80 × 2 = 160 MHz
     * 
     * FLASH LATENCY: 5 wait states for 160 MHz
     *   Flash memory cannot keep up with CPU at high speeds
     *   Wait states insert pauses during Flash reads
     *   STM32U585 requires 5 WS for 160 MHz
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | 
                                   RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | 
                                   RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    
    /* 
     * Flash latency configuration
     * MUST be set before or with clock configuration
     * 
     * Voltage Scaling Range 0: For frequencies > 110 MHz
     * Requires supply voltage > 1.2V (usually 1.8V or 3.3V)
     */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }

    /* --------------------------------------------------------------------
     * STEP 3: Enable peripheral clocks
     * --------------------------------------------------------------------
     * 
     * Each peripheral has its own clock gate to save power.
     * We must enable clocks before accessing peripheral registers.
     * 
     * ORDER MATTERS: Enable clock BEFORE configuring peripheral.
     * If you configure a peripheral with clock disabled,
     * writes go to /dev/null (silently ignored).
     */
    
    /* GPIO clocks - all ports we use */
    __HAL_RCC_GPIOA_CLK_ENABLE();    /* PA0-PA15: ADC, UART1, Motor PWM */
    __HAL_RCC_GPIOB_CLK_ENABLE();    /* Reserved for future use */
    __HAL_RCC_GPIOC_CLK_ENABLE();    /* Reserved for future use */
    __HAL_RCC_GPIOD_CLK_ENABLE();    /* PD5, PD6: UART2 */
    __HAL_RCC_GPIOE_CLK_ENABLE();    /* PE1: Status LED */
    
    /* ADC1 clock */
    __HAL_RCC_ADC_CLK_ENABLE();
    
    /* USART1 and USART2 clocks */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    
    /* Timer clocks for PWM */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    /* GPDMA1 clock (critical for DMA operations) */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* SYSCFG clock (for DMA request mapping) */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
}

/* ============================================================================
 * GPIO INITIALIZATION
 * ============================================================================
 * 
 * Configure GPIO pins that are not part of specific peripheral init.
 * This includes:
 * - Status LED
 * - Motor direction pins
 * 
 * GPIO CONFIGURATION STEPS:
 * 1. Enable GPIO port clock (done in SystemClock_Config)
 * 2. Configure pin mode (input, output, alternate function, analog)
 * 3. Configure output type (push-pull or open-drain)
 * 4. Configure speed (low, medium, high, very high)
 * 5. Configure pull-up/pull-down resistors
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* --------------------------------------------------------------------
     * STATUS LED (PE1)
     * --------------------------------------------------------------------
     * Mode: Output push-pull
     * Speed: Low (LED doesn't need fast switching)
     * Pull: No pull (driven actively)
     */
    GPIO_InitStruct.Pin = STATUS_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &GPIO_InitStruct);
    
    /* Start with LED off (active low on this board) */
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);

    /* --------------------------------------------------------------------
     * MOTOR DIRECTION PINS (PA6, PA7)
     * --------------------------------------------------------------------
     * Mode: Output push-pull
     * Speed: Medium (switched during operation)
     * Pull: No pull
     * Initial state: LOW (forward direction)
     */
    GPIO_InitStruct.Pin = MOTOR_A_DIR_PIN | MOTOR_B_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Set initial direction to forward */
    HAL_GPIO_WritePin(MOTOR_A_DIR_PORT, MOTOR_A_DIR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_B_DIR_PORT, MOTOR_B_DIR_PIN, GPIO_PIN_RESET);
}

/* ============================================================================
 * ERROR HANDLER
 * ============================================================================
 * 
 * Called when a critical error occurs.
 * In production, this would:
 * - Log error to non-volatile memory
 * - Activate visual fault indicator
 * - Possibly trigger system reset
 * 
 * For now, enters infinite loop with LED blinking.
 */
static void Error_Handler(void)
{
    /* Disable interrupts to ensure clean state */
    __disable_irq();
    
    /* Infinite loop with LED indication */
    while (1)
    {
        /* Fast blink to indicate error */
        HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
        for (volatile uint32_t i = 0; i < 500000; i++) { /* Delay */ }
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief Assert failed callback
 * 
 * Called when assert_param macro fails.
 * Useful for debugging parameter validation failures.
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add custom implementation */
}
#endif
