/**
 * @file    freertos_tasks.c
 * @brief   FreeRTOS Task Creation and Implementations
 * 
 * BYPASS MECHANISM NOTE:
 * All timing uses `vTaskDelay(pdMS_TO_TICKS(ms))`. 
 * This function puts the task to sleep, allowing the lower-priority tasks to run.
 * This is fundamentally different from `HAL_Delay()` which spins in a loop
 * burning CPU cycles and blocking the scheduler.
 */

#include "freertos_tasks.h"
#include "adc_manager.h"
#include "filter.h"
#include "state_machine.h"
#include "motor_control.h"
#include "uart_dma.h"
#include "bluetooth.h"
#include "safety.h"
#include "stm32u5xx_hal.h"

/* ============================================================================
 * GLOBAL HANDLES & QUEUES (Defined here, declared extern in header)
 * ============================================================================
 */
TaskHandle_t xSafetyTaskHandle = NULL;
TaskHandle_t xMotorTaskHandle = NULL;
TaskHandle_t xAdcTaskHandle = NULL;
TaskHandle_t xCommTaskHandle = NULL;
TaskHandle_t xBluetoothTaskHandle = NULL;
TaskHandle_t xWatchdogTaskHandle = NULL;

QueueHandle_t xUartRxQueue = NULL;
QueueHandle_t xCommandQueue = NULL;

EventGroupHandle_t xSystemEventGroup = NULL;
SemaphoreHandle_t xSensorDataMutex = NULL;

/* Shared sensor data structure */
static ADC_FilteredData_t current_sensor_data = {0};
static GaitState_t current_gait_state = STATE_IDLE;

/* ============================================================================
 * TASK CREATION
 * ============================================================================
 */
BaseType_t Tasks_CreateAll(void)
{
    BaseType_t status = pdPASS;
    
    /* Create Queues */
    xUartRxQueue = xQueueCreate(QUEUE_UART_RX_SIZE, sizeof(UartRxPacket_t));
    xCommandQueue = xQueueCreate(QUEUE_COMMAND_SIZE, sizeof(ProtocolCommand_t));
    
    if (xUartRxQueue == NULL || xCommandQueue == NULL) return pdFAIL;
    
    /* Create Event Group */
    xSystemEventGroup = xEventGroupCreate();
    if (xSystemEventGroup == NULL) return pdFAIL;
    
    /* Create Mutex */
    xSensorDataMutex = xSemaphoreCreateMutex();
    if (xSensorDataMutex == NULL) return pdFAIL;
    
    /* Create Tasks */
    status &= xTaskCreate(vSafetyTask, "Safety", TASK_STACK_SAFETY, NULL, TASK_PRIORITY_SAFETY, &xSafetyTaskHandle);
    status &= xTaskCreate(vMotorTask, "Motor", TASK_STACK_MOTOR, NULL, TASK_PRIORITY_MOTOR, &xMotorTaskHandle);
    status &= xTaskCreate(vAdcTask, "ADC", TASK_STACK_ADC, NULL, TASK_PRIORITY_ADC, &xAdcTaskHandle);
    status &= xTaskCreate(vCommTask, "Comm", TASK_STACK_COMM, NULL, TASK_PRIORITY_COMM, &xCommTaskHandle);
    status &= xTaskCreate(vBluetoothTask, "BT", TASK_STACK_BLUETOOTH, NULL, TASK_PRIORITY_BLUETOOTH, &xBluetoothTaskHandle);
    status &= xTaskCreate(vWatchdogTask, "WDog", TASK_STACK_WATCHDOG, NULL, TASK_PRIORITY_WATCHDOG, &xWatchdogTaskHandle);
    
    return status;
}

/* ============================================================================
 * TASK IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Safety Task (1ms, Highest Priority)
 */
void vSafetyTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    /* Signal that task is running */
    xEventGroupSetBits(xSystemEventGroup, EVENT_ALL_PERIPHERALS);
    
    for (;;)
    {
        /* Run safety checks */
        Safety_Update(&current_sensor_data);
        
        /* Prove this task is alive to the watchdog mechanism */
        Safety_RefreshTaskWatchdog();
        
        /* Strict 1ms period */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_SAFETY_MS));
    }
}

/**
 * @brief Motor Control Task (1ms)
 */
void vMotorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for (;;)
    {
        /* If in fault, motors are already stopped by safety task */
        if (!Safety_IsFaultActive())
        {
            /* Update PWM ramping towards targets */
            Motor_UpdateRamp();
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_MOTOR_MS));
    }
}

/**
 * @brief ADC Processing Task (5ms)
 */
void vAdcTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ADC_FilteredData_t local_data;
    ADC_RawData_t raw_data;
    
    for (;;)
    {
        /* 1. Get filtered physical values */
        if (ADC_GetFilteredValue(&local_data) == HAL_OK)
        {
            /* 2. Update State Machine */
            current_gait_state = StateMachine_Update(&local_data);
            local_data.state_id = (uint8_t)current_gait_state;
            local_data.spool_angle = Motor_GetStatus(MOTOR_A)->current_duty / 10; /* Approximate spool pos */
            
            /* 3. Safely copy to shared struct (protected by mutex) */
            if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(2)) == pdTRUE)
            {
                current_sensor_data = local_data;
                xSemaphoreGive(xSensorDataMutex);
            }
            
            /* 4. Validate ADC for safety module */
            ADC_GetRawValue(&raw_data);
            /* safety_status.adc_valid = ADC_IsDataValid(&raw_data); */ /* Optional direct set */
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_ADC_MS));
    }
}

/**
 * @brief DMC Communication Task (10ms)
 */
void vCommTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    UartRxPacket_t rx_packet;
    
    for (;;)
    {
        /* Check for packets from DMC (UART2) */
        if (UART_DMA_GetRxPacket(&rx_packet))
        {
            if (rx_packet.instance == UART_INSTANCE_2)
            {
                /* Process DMC packet... */
                Safety_RefreshCommWatchdog();
            }
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_COMM_MS));
    }
}

/**
 * @brief Bluetooth Telemetry Task (50ms = 20Hz)
 */
void vBluetoothTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ADC_FilteredData_t local_telemetry;
    ProtocolCommand_t cmd;
    
    for (;;)
    {
        /* 1. Process Incoming Commands */
        cmd = Bluetooth_ProcessRx();
        if (cmd == CMD_RESET_FAULT) {
            Safety_ClearFault();
            StateMachine_Reset();
        } else if (cmd == CMD_START_CALIBRATION) {
            StateMachine_ForceState(STATE_CALIBRATION);
        }
        
        /* 2. Transmit Telemetry */
        if (Bluetooth_IsConnected() && !Safety_IsFaultActive())
        {
            /* Safely copy shared data */
            if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
            {
                local_telemetry = current_sensor_data;
                xSemaphoreGive(xSensorDataMutex);
                
                /* Send over Bluetooth */
                Bluetooth_SendTelemetry(&local_telemetry, current_gait_state);
            }
        }
        
        /* 3. If in fault, send error continuously */
        if (Safety_IsFaultActive())
        {
            Bluetooth_SendError(Safety_GetStatus()->last_error);
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_BLUETOOTH_MS));
    }
}

/**
 * @brief Watchdog Task (100ms)
 */
void vWatchdogTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for (;;)
    {
        /* 
         * In a full implementation, we would check if all task handles are valid
         * and optionally refresh the hardware IWDG here.
         * For now, it serves as a heartbeat monitor.
         */
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_WATCHDOG_MS));
    }
}
