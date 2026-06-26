/**
 * @file    stm32u5xx_it.c
 * @brief   Interrupt Service Routines
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Contains all interrupt handlers for the system.
 * 
 * CRITICAL DESIGN RULES:
 * 1. Keep ISRs as short as possible
 * 2. Never call blocking functions in ISRs
 * 3. Use "FromISR" FreeRTOS API functions
 * 4. Clear interrupt flags before exiting
 * 
 * FREERTOS VECTOR MIGRATION:
 * FreeRTOS requires specific handlers for context switching:
 * - SVC_Handler: Triggers first task start
 * - PendSV_Handler: Performs context switch
 * - SysTick_Handler: Triggers scheduler tick
 * 
 * We place these in USER CODE sections to prevent
 * CubeMX/sweep tools from deleting them.
 * ============================================================================
 */

/* Includes */
#include "main.h"
#include "stm32u5xx_it.h"
#include "FreeRTOS.h"
#include "task.h"

/* External variables - declared in peripheral init modules */
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;

/* ============================================================================
 * USER CODE BEGIN 0 - FreeRTOS Vector Declarations
 * ============================================================================
 * 
 * These declarations tell the linker that these symbols exist
 * and should be used for the corresponding interrupt vectors.
 * 
 * The actual implementations are in the portable layer
 * (port.c in ARM_CM33_NTZ folder), but we need to ensure
 * the vector table points to them, not to default handlers.
 * 
 * Why extern? The functions are defined in FreeRTOS portable code.
 * We declare them here so we can create wrapper handlers if needed.
 */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
/* USER CODE END 0 */

/* ============================================================================
 * SYSTEM EXCEPTION HANDLERS
 * ============================================================================ */

/**
 * @brief Non-Maskable Interrupt Handler
 * 
 * NMI is triggered by:
 * - Clock failure detection
 * - SRAM parity error
 * - External NMI pin
 * 
 * In production, log error and attempt safe shutdown.
 */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
    while (1)
    {
        /* Trap here - NMI indicates serious hardware problem */
    }
    /* USER CODE END NonMaskableInt_IRQn 0 */
}

/**
 * @brief Hard Fault Handler
 * 
 * Hard fault occurs when:
 * - Dividing by zero
 * - Accessing invalid memory address
 * - Executing undefined instruction
 * - Bus fault escalates to hard fault
 * 
 * DEBUGGING HARD FAULTS:
 * When hard fault occurs, these registers contain clues:
 * - MSP/PSP: Stack pointer at time of fault
 * - LR: Return address (often points to faulting instruction)
 * - CFSR: Configurable Fault Status Register (details)
 * - HFSR: Hard Fault Status Register
 * - BFAR: Bus Fault Address Register
 * 
 * For production, implement fault logging to Flash.
 */
void HardFault_Handler(void)
{
    /* USER CODE BEGIN HardFault_IRQn 0 */
    while (1)
    {
        /* 
         * In production, read fault registers and log to Flash:
         * uint32_t cfsr = SCB->CFSR;
         * uint32_t hfsr = SCB->HFSR;
         * uint32_t bfar = SCB->BFAR;
         * uint32_t pc = __get_PC();
         * 
         * Then attempt safe motor shutdown if possible.
         */
    }
    /* USER CODE END HardFault_IRQn 0 */
}

/**
 * @brief Memory Management Fault Handler
 * 
 * Caused by invalid MPU configuration or access violations.
 */
void MemManage_Handler(void)
{
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */
    while (1)
    {
    }
    /* USER CODE END MemoryManagement_IRQn 0 */
}

/**
 * @brief Bus Fault Handler
 * 
 * Caused by invalid memory access (e.g., accessing peripheral
 * that doesn't exist, or unaligned access where not allowed).
 */
void BusFault_Handler(void)
{
    /* USER CODE BEGIN BusFault_IRQn 0 */
    while (1)
    {
    }
    /* USER CODE END BusFault_IRQn 0 */
}

/**
 * @brief Usage Fault Handler
 * 
 * Caused by:
 * - Undefined instruction
 * - Invalid state (Thumb bit incorrect)
 * - Division by zero
 * - Unaligned access
 */
void UsageFault_Handler(void)
{
    /* USER CODE BEGIN UsageFault_IRQn 0 */
    while (1)
    {
    }
    /* USER CODE END UsageFault_IRQn 0 */
}

/**
 * @brief Debug Monitor Handler
 * 
 * Used for debug features like breakpoints and watchpoints.
 * Not used in production firmware.
 */
void DebugMon_Handler(void)
{
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */
    /* Empty - not used */
    /* USER CODE END DebugMonitor_IRQn 0 */
}

/* ============================================================================
 * FREERTOS CONTEXT SWITCHING HANDLERS
 * ============================================================================
 * 
 * These handlers are CRITICAL for FreeRTOS operation.
 * They must NOT be modified or wrapped.
 * 
 * SVC_Handler: System Service Call
 *   - Called by first task to start scheduling
 *   - Uses SVC instruction to switch from handler to thread mode
 * 
 * PendSV_Handler: Pending SV Call
 *   - Lowest priority exception
 *   - Used for context switching between tasks
 *   - Runs when no other exception is active
 * 
 * SysTick_Handler: System Tick Timer
 *   - Called every 1 ms by SysTick timer
 *   - Increments tick count
 *   - Checks if any task needs to be woken
 *   - Triggers PendSV if context switch needed
 */

/**
 * @brief SVC Handler - FreeRTOS Vector
 * 
 * This handler is provided by FreeRTOS portable layer.
 * We create a wrapper that calls the FreeRTOS implementation.
 * 
 * IMPORTANT: Do not add any code here!
 */
void SVC_Handler(void)
{
    /* USER CODE BEGIN SVCall_IRQn 0 */
    vPortSVCHandler();
    /* USER CODE END SVCall_IRQn 0 */
}

/**
 * @brief PendSV Handler - FreeRTOS Vector
 * 
 * This handler performs the actual context switch.
 * It saves current task state and restores next task state.
 * 
 * IMPORTANT: Do not add any code here!
 */
void PendSV_Handler(void)
{
    /* USER CODE BEGIN PendSV_IRQn 0 */
    xPortPendSVHandler();
    /* USER CODE END PendSV_IRQn 0 */
}

/**
 * @brief SysTick Handler - FreeRTOS Vector
 * 
 * Called every 1 ms. Previously, this called HAL_IncTick()
 * to support HAL_Delay(). We removed that because:
 * 1. We don't use HAL_Delay() (we use vTaskDelay())
 * 2. HAL_IncTick() could interfere with FreeRTOS timing
 * 
 * Now it ONLY calls FreeRTOS tick handler.
 * 
 * TIMING ACCURACY:
 * SysTick is driven by system clock (160 MHz).
 * LOAD value = (160000000 / 1000) - 1 = 159999
 * This gives exactly 1.000 ms between ticks.
 */
void SysTick_Handler(void)
{
    /* USER CODE BEGIN SysTick_IRQn 0 */
    /* 
     * CRITICAL: Only call FreeRTOS handler.
     * Do NOT call HAL_IncTick() - it has been removed.
     */
    xPortSysTickHandler();
    /* USER CODE END SysTick_IRQn 0 */
}

/* ============================================================================
 * DMA INTERRUPT HANDLERS
 * ============================================================================ */

/**
 * @brief GPDMA1 Channel 0 Interrupt (ADC)
 * 
 * Triggered when ADC DMA transfer completes or half-completes.
 * 
 * For circular mode:
 * - Half Transfer: First half of buffer is full
 * - Full Transfer: Second half of buffer is full
 * 
 * This allows double-buffering: process one half while
 * the other half is being filled.
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    /* USER CODE BEGIN GPDMA1_Channel0_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_adc1);
    /* USER CODE END GPDMA1_Channel0_IRQn 0 */
}

/**
 * @brief GPDMA1 Channel 1 Interrupt (USART1 TX)
 * 
 * Triggered when UART1 transmit DMA completes.
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    /* USER CODE BEGIN GPDMA1_Channel1_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
    /* USER CODE END GPDMA1_Channel1_IRQn 0 */
}

/**
 * @brief GPDMA1 Channel 2 Interrupt (USART1 RX)
 * 
 * Triggered when:
 * - Receive complete (idle line detected)
 * - Half transfer
 * - Error
 */
void GPDMA1_Channel2_IRQHandler(void)
{
    /* USER CODE BEGIN GPDMA1_Channel2_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
    /* USER CODE END GPDMA1_Channel2_IRQn 0 */
}

/**
 * @brief GPDMA1 Channel 3 Interrupt (USART2 TX)
 */
void GPDMA1_Channel3_IRQHandler(void)
{
    /* USER CODE BEGIN GPDMA1_Channel3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
    /* USER CODE END GPDMA1_Channel3_IRQn 0 */
}

/**
 * @brief GPDMA1 Channel 4 Interrupt (USART2 RX)
 */
void GPDMA1_Channel4_IRQHandler(void)
{
    /* USER CODE BEGIN GPDMA1_Channel4_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
    /* USER CODE END GPDMA1_Channel4_IRQn 0 */
}

/* ============================================================================
 * UART INTERRUPT HANDLERS
 * ============================================================================ */

/**
 * @brief USART1 Interrupt Handler
 * 
 * Handles USART1 events including:
 * - Idle line detection (packet end)
 * - Transmission complete
 * - Parity error
 * - Framing error
 * - Overrun error
 * 
 * The HAL_UART_IRQHandler function checks status registers
 * and calls appropriate callbacks.
 */
void USART1_IRQHandler(void)
{
    /* USER CODE BEGIN USART1_IRQn 0 */
    HAL_UART_IRQHandler(&huart1);
    /* USER CODE END USART1_IRQn 0 */
}

/**
 * @brief USART2 Interrupt Handler
 */
void USART2_IRQHandler(void)
{
    /* USER CODE BEGIN USART2_IRQn 0 */
    HAL_UART_IRQHandler(&huart2);
    /* USER CODE END USART2_IRQn 0 */
}

/* ============================================================================
 * ADC INTERRUPT HANDLER
 * ============================================================================ */

/**
 * @brief ADC1 Interrupt Handler
 * 
 * Handles ADC events:
 * - End of conversion
 * - End of sequence
 * - Analog watchdog
 * - Overrun
 * 
 * Note: With DMA, we typically don't need this interrupt
 * because DMA handles data transfer. But we enable it
 * for error detection.
 */
void ADC1_IRQHandler(void)
{
    /* USER CODE BEGIN ADC1_IRQn 0 */
    HAL_ADC_IRQHandler(&hadc1);
    /* USER CODE END ADC1_IRQn 0 */
}
