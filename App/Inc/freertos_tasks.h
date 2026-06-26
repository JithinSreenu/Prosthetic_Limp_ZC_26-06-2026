/**
 * @file    freertos_tasks.h
 * @brief   FreeRTOS Task Definitions Header
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * 
 * ============================================================================
 * PURPOSE:
 * Defines all FreeRTOS tasks and their interfaces.
 * 
 * FREERTOS TASK MODEL:
 * Each task is an independent execution thread with:
 * - Own stack space
 * - Own priority
 * - Periodic or event-driven execution
 * 
 * TASK HIERARCHY (Priority Order):
 * 
 * ┌─────────────────────────────────────────┐
 * │  Safety Task (Priority 6, 1ms period)   │ ← Highest - always runs
 * ├─────────────────────────────────────────┤
 * │  Motor Task (Priority 5, 1ms period)    │
 * ├─────────────────────────────────────────┤
 * │  ADC Task (Priority 4, 5ms period)      │
 * ├─────────────────────────────────────────┤
 * │  Comm Task (Priority 3, 10ms period)    │
 * ├─────────────────────────────────────────┤
 * │  Bluetooth Task (Priority 2, 50ms)      │
 * ├─────────────────────────────────────────┤
 * │  Watchdog Task (Priority 1, 100ms)      │ ← Lowest
 * └─────────────────────────────────────────┘
 * 
 * INTER-TASK COMMUNICATION:
 * - Queues: For passing data packets between tasks
 * - Event Flags: For signaling conditions
 * - Task Notifications: For lightweight signaling
 * - Shared Variables: Protected by mutexes (minimized)
 * ============================================================================
 */

#ifndef FREERTOS_TASKS_H
#define FREERTOS_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "app_config.h"

/* ============================================================================
 * TASK HANDLE DECLARATIONS
 * ============================================================================
 * 
 * Task handles allow external control of tasks:
 * - vTaskSuspend(): Pause task
 * - vTaskResume(): Resume task
 * - vTaskDelete(): Terminate task
 * - xTaskNotify(): Send notification to task
 * 
 * Handles are initialized during task creation.
 */

extern TaskHandle_t xSafetyTaskHandle;
extern TaskHandle_t xMotorTaskHandle;
extern TaskHandle_t xAdcTaskHandle;
extern TaskHandle_t xCommTaskHandle;
extern TaskHandle_t xBluetoothTaskHandle;
extern TaskHandle_t xWatchdogTaskHandle;

/* ============================================================================
 * QUEUE HANDLE DECLARATIONS
 * ============================================================================
 * 
 * Queues enable safe data passing between tasks.
 * FreeRTOS queues are thread-safe - no additional
 * synchronization needed.
 * 
 * UART RX Queue: Holds received packets for processing
 * Command Queue: Holds decoded commands for execution
 */

extern QueueHandle_t xUartRxQueue;
extern QueueHandle_t xCommandQueue;

/* ============================================================================
 * EVENT GROUP HANDLE
 * ============================================================================
 * 
 * Event groups allow multiple bits to be set/waited atomically.
 * Used for system initialization synchronization.
 */

extern EventGroupHandle_t xSystemEventGroup;

/* Event group bits */
#define EVENT_ADC_READY       (1U << 0)    /* ADC initialized and running */
#define EVENT_UART1_READY     (1U << 1)    /* USART1 initialized */
#define EVENT_UART2_READY     (1U << 2)    /* USART2 initialized */
#define EVENT_MOTOR_READY     (1U << 3)    /* Motors initialized */
#define EVENT_CALIBRATION_DONE (1U << 4)   /* Calibration complete */
#define EVENT_ALL_PERIPHERALS (EVENT_ADC_READY | EVENT_UART1_READY | \
                               EVENT_UART2_READY | EVENT_MOTOR_READY)

/* ============================================================================
 * MUTEX HANDLE DECLARATIONS
 * ============================================================================
 * 
 * Mutexes protect shared resources from concurrent access.
 * We minimize shared state, but some is unavoidable.
 */

extern SemaphoreHandle_t xSensorDataMutex;

/* ============================================================================
 * TASK FUNCTION PROTOTYPES
 * ============================================================================
 * 
 * Each task function has standard FreeRTOS signature:
 * void TaskFunction(void *pvParameters)
 * 
 * Tasks typically run infinite loops with vTaskDelay()
 * for periodic execution.
 */

/**
 * @brief Safety monitoring task
 * 
 * Priority: 6 (highest)
 * Period: 1 ms
 * Stack: 128 words
 * 
 * Responsibilities:
 * - Check ADC validity
 * - Monitor communication timeouts
 * - Check battery voltage
 * - Trigger faults if needed
 * - Refresh hardware watchdog
 */
void vSafetyTask(void *pvParameters);

/**
 * @brief Motor control task
 * 
 * Priority: 5
 * Period: 1 ms
 * Stack: 256 words
 * 
 * Responsibilities:
 * - Update motor duty cycle ramping
 * - Execute valve control commands
 * - Monitor motor status
 * - Implement current limiting
 */
void vMotorTask(void *pvParameters);

/**
 * @brief ADC processing task
 * 
 * Priority: 4
 * Period: 5 ms
 * Stack: 256 words
 * 
 * Responsibilities:
 * - Read DMA buffer
 * - Apply filters
 * - Convert to physical units
 * - Update state machine
 * - Provide data to other tasks
 */
void vAdcTask(void *pvParameters);

/**
 * @brief DMC communication task
 * 
 * Priority: 3
 * Period: 10 ms
 * Stack: 256 words
 * 
 * Responsibilities:
 * - Process USART2 received packets
 * - Send commands to DMC
 * - Handle DMC protocol
 * - Refresh communication watchdog
 */
void vCommTask(void *pvParameters);

/**
 * @brief Bluetooth telemetry task
 * 
 * Priority: 2
 * Period: 50 ms
 * Stack: 512 words
 * 
 * Responsibilities:
 * - Send telemetry packets
 * - Process Bluetooth commands
 * - Handle configuration requests
 * - Send status/error notifications
 */
void vBluetoothTask(void *pvParameters);

/**
 * @brief Watchdog refresh task
 * 
 * Priority: 1 (lowest)
 * Period: 100 ms
 * Stack: 128 words
 * 
 * Responsibilities:
 * - Verify all tasks are running
 * - Refresh independent watchdog (IWDG)
 * - Log system health
 * - Detect task starvation
 */
void vWatchdogTask(void *pvParameters);

/* ============================================================================
 * TASK CREATION FUNCTION
 * ============================================================================
 */

/**
 * @brief Create all FreeRTOS tasks and synchronization objects
 * 
 * Creates:
 * - All task handles
 * - All queues
 * - Event groups
 * - Mutexes
 * 
 * @return BaseType_t pdPASS if all created successfully
 */
BaseType_t Tasks_CreateAll(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_TASKS_H */
