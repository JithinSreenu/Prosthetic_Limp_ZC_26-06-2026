/**
 * @file    app_config.h
 * @brief   Master Configuration Header for Prosthetic Knee Controller
 * @author  Embedded Systems Architect
 * @version 1.0.0
 * @date    2024
 * 
 * ============================================================================
 * PURPOSE:
 * This file contains all system-wide configuration parameters, constants,
 * and compile-time options for the prosthetic knee controller firmware.
 * 
 * ARCHITECTURE EXPLANATION:
 * In a production embedded system, magic numbers (raw numeric literals) 
 * scattered throughout code are a major maintenance hazard. This header
 * centralizes all configurable parameters in one location, making:
 * - System tuning easier (change one place, affects whole system)
 * - Code more readable (names instead of numbers)
 * - Validation simpler (all limits defined once)
 * - Certification smoother (traceability to requirements)
 * 
 * USAGE:
 * All source files include this header to access system parameters.
 * Modify values here to adjust system behavior without touching logic.
 * ============================================================================
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INCLUDES
 * ============================================================================
 * Standard integer types from CMSIS for portability across compilers.
 * uint8_t, uint16_t, int16_t, etc. are guaranteed sizes.
 */
#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * VERSION INFORMATION
 * ============================================================================
 * Version tracking is critical for production devices.
 * Allows field technicians to verify firmware version via Bluetooth.
 * Format: MAJOR.MINOR.PATCH
 * - MAJOR: Incompatible API changes
 * - MINOR: Backward-compatible functionality additions
 * - PATCH: Backward-compatible bug fixes
 */
#define FW_VERSION_MAJOR        (1U)
#define FW_VERSION_MINOR        (0U)
#define FW_VERSION_PATCH        (0U)
#define FW_VERSION_STRING       "1.0.0"

/* ============================================================================
 * HARDWARE PIN DEFINITIONS
 * ============================================================================
 * The STM32U585 has multiple GPIO ports (A, B, C, D, etc.).
 * Each pin can be configured for alternate functions (USART, TIM, ADC).
 * 
 * PIN NUMBERING CONVENTION:
 * GPIO_PIN_0  = Pin 0 of the port
 * GPIO_PIN_5  = Pin 5 of the port
 * GPIO_PIN_9  = Pin 9 of the port
 * 
 * These definitions map logical functions to physical pins.
 * If hardware changes, only these definitions need updating.
 * ============================================================================
 */

/* -------------------------------------------------------------------------
 * ADC Input Pins
 * -------------------------------------------------------------------------
 * The STM32U585 ADC1 can read from multiple channels.
 * Each channel corresponds to a specific pin.
 * Channel numbers are fixed by silicon design.
 * 
 * PIN SELECTION RATIONALE:
 * - PA0 (ADC1_IN5): Close to load cell amplifier output
 * - PA1 (ADC1_IN6): Dedicated angle sensor input
 * - PA2 (ADC1_IN7): Moment sensor analog output
 * - PA3 (ADC1_IN8): Battery voltage divider output
 * -------------------------------------------------------------------------
 */
#define ADC_LOAD_CELL_PIN       GPIO_PIN_0      /* PA0 - Load cell force signal */
#define ADC_LOAD_CELL_PORT      GPIOA           /* Port A */
#define ADC_LOAD_CELL_CHANNEL   ADC_CHANNEL_5   /* ADC1 Channel 5 */

#define ADC_KNEE_ANGLE_PIN      GPIO_PIN_1      /* PA1 - Knee angle sensor */
#define ADC_KNEE_ANGLE_PORT     GPIOA
#define ADC_KNEE_ANGLE_CHANNEL  ADC_CHANNEL_6   /* ADC1 Channel 6 */

#define ADC_MOMENT_PIN          GPIO_PIN_2      /* PA2 - Moment/torque sensor */
#define ADC_MOMENT_PORT         GPIOA
#define ADC_MOMENT_CHANNEL      ADC_CHANNEL_7   /* ADC1 Channel 7 */

#define ADC_BATTERY_PIN         GPIO_PIN_3      /* PA3 - Battery voltage */
#define ADC_BATTERY_PORT        GPIOA
#define ADC_BATTERY_CHANNEL     ADC_CHANNEL_8   /* ADC1 Channel 8 */

/* -------------------------------------------------------------------------
 * USART1 Pins (Bluetooth Communication)
 * -------------------------------------------------------------------------
 * USART1 is connected to the Bluetooth module (HC-05 or similar).
 * TX pin transmits data FROM microcontroller TO Bluetooth module.
 * RX pin receives data FROM Bluetooth module TO microcontroller.
 * 
 * CROSS-CONNECTION RULE:
 * STM32 TX connects to Bluetooth RX
 * STM32 RX connects to Bluetooth TX
 * 
 * PIN ALTERNATE FUNCTION:
 * AF7 is the USART alternate function for PA9/PA10 on STM32U585.
 * This tells the GPIO multiplexer to route these pins to USART1.
 * -------------------------------------------------------------------------
 */
#define USART1_TX_PIN           GPIO_PIN_9      /* PA9  - Transmit */
#define USART1_TX_PORT          GPIOA
#define USART1_RX_PIN           GPIO_PIN_10     /* PA10 - Receive */
#define USART1_RX_PORT          GPIOA
#define USART1_AF               GPIO_AF7_USART1 /* Alternate Function 7 */

/* -------------------------------------------------------------------------
 * USART2 Pins (DMC External Controller)
 * -------------------------------------------------------------------------
 * USART2 communicates with the Damper Motor Controller (DMC).
 * Different pins than USART1 to avoid conflict.
 * 
 * PIN SELECTION:
 * PD5/PD6 are default USART2 pins on B-U585I-IOT02A board.
 * These pins have appropriate routing on the PCB.
 * -------------------------------------------------------------------------
 */
#define USART2_TX_PIN           GPIO_PIN_5      /* PD5  - Transmit to DMC */
#define USART2_TX_PORT          GPIOD
#define USART2_RX_PIN           GPIO_PIN_6      /* PD6  - Receive from DMC */
#define USART2_RX_PORT          GPIOD
#define USART2_AF               GPIO_AF7_USART2 /* Alternate Function 7 */

/* -------------------------------------------------------------------------
 * Motor PWM Pins
 * -------------------------------------------------------------------------
 * Two motors control hydraulic valves:
 * - Motor A: Opens the valve (allows fluid flow for knee flexion)
 * - Motor B: Closes the valve (restricts flow for knee extension support)
 * 
 * TIMER SELECTION:
 * TIM1 is an advanced-control timer with:
 * - Complementary outputs (not used here but available)
 * - Dead-time insertion (prevents shoot-through in H-bridges)
 * - Break input (emergency stop capability)
 * 
 * CHANNEL ASSIGNMENT:
 * TIM1_CH1 on PA8 drives Motor A
 * TIM1_CH2 on PA9... WAIT, PA9 is already USART1_TX!
 * 
 * CORRECTION: We must use different pins for PWM.
 * TIM1_CH1: PA8 (confirmed available)
 * TIM1_CH2: PA9 conflicts with USART1_TX
 * 
 * SOLUTION: Use TIM2 for the second motor or remap TIM1.
 * Let's use TIM2_CH1 on PA0... but PA0 is ADC!
 * 
 * FINAL SOLUTION: 
 * TIM1_CH1: PA8 for Motor A
 * TIM1_CH2: PE11 for Motor B (remapped pin)
 * OR
 * TIM2_CH1: PA15 for Motor B
 * 
 * For simplicity on B-U585I-IOT02A:
 * Motor A: TIM1_CH1 on PA8
 * Motor B: TIM2_CH1 on PA15
 * -------------------------------------------------------------------------
 */
#define MOTOR_A_PWM_PIN         GPIO_PIN_8      /* PA8  - Motor A PWM */
#define MOTOR_A_PWM_PORT        GPIOA
#define MOTOR_A_PWM_AF          GPIO_AF1_TIM1   /* TIM1 AF */
#define MOTOR_A_PWM_CHANNEL     TIM_CHANNEL_1   /* TIM1 Channel 1 */

#define MOTOR_B_PWM_PIN         GPIO_PIN_15     /* PA15 - Motor B PWM */
#define MOTOR_B_PWM_PORT        GPIOA
#define MOTOR_B_PWM_AF          GPIO_AF1_TIM2   /* TIM2 AF */
#define MOTOR_B_PWM_CHANNEL     TIM_CHANNEL_1   /* TIM2 Channel 1 */

/* -------------------------------------------------------------------------
 * Motor Direction Control Pins
 * -------------------------------------------------------------------------
 * H-bridge direction pins control motor rotation direction.
 * These are simple GPIO outputs, not PWM.
 * 
 * H-BRIDGE TRUTH TABLE:
 * DIR_A=0, DIR_B=0: Motor stopped (brake)
 * DIR_A=1, DIR_B=0: Motor forward
 * DIR_A=0, DIR_B=1: Motor backward
 * DIR_A=1, DIR_B=1: Motor stopped (coast) - AVOID THIS
 * -------------------------------------------------------------------------
 */
#define MOTOR_A_DIR_PIN         GPIO_PIN_7      /* PA7  - Motor A direction */
#define MOTOR_A_DIR_PORT        GPIOA
#define MOTOR_B_DIR_PIN         GPIO_PIN_6      /* PA6  - Motor B direction */
#define MOTOR_B_DIR_PORT        GPIOA

/* -------------------------------------------------------------------------
 * Status LED Pin
 * -------------------------------------------------------------------------
 * Debug/status indicator LED.
 * On B-U585I-IOT02A, LD1 is on PE1.
 * -------------------------------------------------------------------------
 */
#define STATUS_LED_PIN          GPIO_PIN_1      /* PE1 - Status LED */
#define STATUS_LED_PORT         GPIOE

/* ============================================================================
 * UART COMMUNICATION PARAMETERS
 * ============================================================================
 * BAUD RATE:
 * 115200 bits per second is the standard for:
 * - Bluetooth SPP (Serial Port Profile)
 * - Most USB-TTL adapters
 * - Balance between speed and reliability
 * 
 * At 115200 bps:
 * - Each bit takes ~8.68 microseconds
 * - A 13-byte packet takes ~113 microseconds
 * - At 50 Hz telemetry, we use only 0.56% of bandwidth
 * 
 * FRAME FORMAT (8N1):
 * - 8 data bits: Standard for ASCII and binary data
 * - No parity: Reduces overhead, error checking via checksum
 * - 1 stop bit: Standard configuration
 * 
 * TOTAL BITS PER BYTE: 1 (start) + 8 (data) + 1 (stop) = 10 bits
 * ============================================================================
 */
#define UART_BAUD_RATE          (115200U)
#define UART_WORD_LENGTH        UART_WORDLENGTH_8B
#define UART_STOP_BITS          UART_STOPBITS_1
#define UART_PARITY             UART_PARITY_NONE
#define UART_HW_FLOW_CTRL       UART_HWCONTROL_NONE
#define UART_OVERSAMPLING       UART_OVERSAMPLING_16

/* ============================================================================
 * DMA CONFIGURATION
 * ============================================================================
 * DMA (Direct Memory Access) transfers data between peripherals and memory
 * without CPU intervention. This is CRITICAL for real-time systems.
 * 
 * GPDMA vs Legacy DMA:
 * STM32U585 has GPDMA (General Purpose DMA) which is more advanced than
 * the legacy DMA in older STM32 families. GPDMA supports:
 * - 2D addressing (useful for display buffers)
 * - Linked lists (chaining transfers)
 * - More channels and requests
 * 
 * CHANNEL ALLOCATION:
 * Each DMA request line needs its own channel.
 * We allocate non-conflicting channels for each peripheral.
 * 
 * CIRCULAR MODE (ADC):
 * DMA automatically wraps around to buffer start when it reaches the end.
 * This creates a continuous capture without CPU intervention.
 * 
 * NORMAL MODE (UART TX):
 * DMA transfers exactly the requested bytes and stops.
 * Must be re-armed for each transmission.
 * ============================================================================
 */

/* ADC DMA Configuration */
#define ADC_DMA_INSTANCE        GPDMA1                 /* GPDMA1 controller */
#define ADC_DMA_CHANNEL         DMA_CHANNEL_0          /* Channel 0 for ADC */
#define ADC_DMA_REQUEST         GPDMA_REQUEST_ADC1     /* ADC1 request line */
#define ADC_DMA_PRIORITY        DMA_HIGH_PRIORITY      /* High priority for sensor data */

/* USART1 DMA Configuration (Bluetooth) */
#define USART1_TX_DMA_INSTANCE  GPDMA1
#define USART1_TX_DMA_CHANNEL   DMA_CHANNEL_1          /* Channel 1 for USART1 TX */
#define USART1_TX_DMA_REQUEST   GPDMA_REQUEST_USART1_TX
#define USART1_TX_DMA_PRIORITY  DMA_MEDIUM_PRIORITY

#define USART1_RX_DMA_INSTANCE  GPDMA1
#define USART1_RX_DMA_CHANNEL   DMA_CHANNEL_2          /* Channel 2 for USART1 RX */
#define USART1_RX_DMA_REQUEST   GPDMA_REQUEST_USART1_RX
#define USART1_RX_DMA_PRIORITY  DMA_MEDIUM_PRIORITY

/* USART2 DMA Configuration (DMC) */
#define USART2_TX_DMA_INSTANCE  GPDMA1
#define USART2_TX_DMA_CHANNEL   DMA_CHANNEL_3          /* Channel 3 for USART2 TX */
#define USART2_TX_DMA_REQUEST   GPDMA_REQUEST_USART2_TX
#define USART2_TX_DMA_PRIORITY  DMA_LOW_PRIORITY

#define USART2_RX_DMA_INSTANCE  GPDMA1
#define USART2_RX_DMA_CHANNEL   DMA_CHANNEL_4          /* Channel 4 for USART2 RX */
#define USART2_RX_DMA_REQUEST   GPDMA_REQUEST_USART2_RX
#define USART2_RX_DMA_PRIORITY  DMA_LOW_PRIORITY

/* ============================================================================
 * ADC CONFIGURATION
 * ============================================================================
 * RESOLUTION:
 * 12-bit resolution provides 4096 discrete levels (0-4095).
 * For a 0-3.3V range: ~0.8mV per LSB
 * This is sufficient for our sensor ranges after signal conditioning.
 * 
 * SAMPLING RATE CALCULATION:
 * Target: 1 kHz per channel (4 channels = 4 kSPS total)
 * With 4 channels in scan mode, ADC must convert at 4 kSPS minimum.
 * STM32U585 ADC can achieve > 4 MSPS, so we have huge margin.
 * 
 * We'll use a slower rate to reduce noise and power consumption.
 * 
 * OVERSAMPLING:
 * Hardware oversampling averages multiple samples per conversion.
 * 16x oversampling improves resolution by 2 bits (effective 14-bit).
 * Also provides hardware noise filtering.
 * ============================================================================
 */
#define ADC_RESOLUTION          ADC_RESOLUTION_12B
#define ADC_DATA_ALIGN          ADC_DATAALIGN_RIGHT    /* Right-align for easy masking */
#define ADC_SCAN_MODE           ENABLE                 /* Scan multiple channels */
#define ADC_CONTINUOUS_MODE     ENABLE                 /* Continuous conversion */
#define ADC_OVERSAMPLING_RATIO  ADC_OVERSAMPLING_RATIO_16X
#define ADC_OVERSAMPLING_SHIFT  ADC_OVERSAMPLING_SHIFT_4  /* 16x = 4 bit shift */

/* Number of ADC channels to scan */
#define ADC_NUM_CHANNELS        (4U)

/* ADC DMA buffer size (must be multiple of ADC_NUM_CHANNELS) */
#define ADC_DMA_BUFFER_SIZE     (ADC_NUM_CHANNELS * 16U)  /* 16 samples per channel */

/* ADC sampling time (clock cycles)
 * Longer sampling time = more accurate but slower.
 * 247.5 cycles at 20 MHz ADC clock = ~12.4 microseconds per channel.
 * 4 channels × 12.4 µs = 49.6 µs per scan
 * Effective rate: ~20 kSPS per channel (more than needed) */
#define ADC_SAMPLE_TIME         ADC_SAMPLETIME_247CYCLES_5

/* ============================================================================
 * SENSOR DATA RANGES AND SCALING
 * ============================================================================
 * These constants define the valid ranges for each sensor.
 * Used for:
 * 1. Scaling raw ADC values to physical units
 * 2. Validating sensor readings (safety check)
 * 3. Optimizing data types for transmission
 * 
 * SCALING FORMULA:
 * Physical_Value = (ADC_Raw - ADC_Min) * (Physical_Max - Physical_Min) / 
 *                  (ADC_Max - ADC_Min) + Physical_Min
 * 
 * For signed values centered at zero:
 * Physical_Value = (ADC_Raw - ADC_Center) * Physical_Range / ADC_Half_Range
 * ============================================================================
 */

/* Load Cell: ±200 N (Newtons)
 * Signal conditioning provides 1.65V at 0N, 0V at -200N, 3.3V at +200N
 * ADC center value: 2048 (half of 4095)
 * Scale: 200 / 2048 ≈ 0.0977 N per LSB */
#define FORCE_ADC_CENTER        (2048.0f)
#define FORCE_PHYSICAL_RANGE    (200.0f)
#define FORCE_ADC_HALF_RANGE    (2048.0f)
#define FORCE_MIN_VALID         (-200)
#define FORCE_MAX_VALID         (200)

/* Knee Angle: ±200 degrees
 * Similar bipolar arrangement as force */
#define KNEE_ANGLE_ADC_CENTER   (2048.0f)
#define KNEE_ANGLE_PHYSICAL_RANGE (200.0f)
#define KNEE_ANGLE_ADC_HALF_RANGE (2048.0f)
#define KNEE_ANGLE_MIN_VALID    (-200)
#define KNEE_ANGLE_MAX_VALID    (200)

/* Moment: ±3000 Nm (Newton-meters)
 * Higher range requires same ADC resolution */
#define MOMENT_ADC_CENTER       (2048.0f)
#define MOMENT_PHYSICAL_RANGE   (3000.0f)
#define MOMENT_ADC_HALF_RANGE   (2048.0f)
#define MOMENT_MIN_VALID        (-3000)
#define MOMENT_MAX_VALID        (3000)

/* Battery: 0 to 5V
 * Voltage divider: 5V → 3.3V (ratio 0.66)
 * ADC full scale (4095) = 5V actual
 * Scale: 5000 / 4095 ≈ 1.221 mV per LSB */
#define BATTERY_MAX_VOLTAGE_MV  (5000U)
#define BATTERY_ADC_MAX         (4095U)
#define BATTERY_MIN_VALID_MV    (3000U)   /* 3.0V minimum - system shut down */
#define BATTERY_WARNING_MV      (3600U)   /* 3.6V warning level */

/* Spool Angle: 0 to 100 (percentage)
 * This is calculated from motor position, not directly from ADC */
#define SPOOL_ANGLE_MIN         (0U)
#define SPOOL_ANGLE_MAX         (100U)

/* ============================================================================
 * FILTER CONFIGURATION
 * ============================================================================
 * Filters remove noise from sensor readings.
 * Different filter types have different characteristics:
 * 
 * MOVING AVERAGE:
 * - Simple to implement
 * - Good for random noise
 * - Introduces delay proportional to window size
 * - Window of 8: 8-sample delay
 * 
 * IIR (Infinite Impulse Response):
 * - Exponential moving average
 * - Single state variable (memory efficient)
 * - Alpha controls responsiveness vs smoothing
 * - Alpha = 0.2: Smooth, 80% weight to history
 * - Alpha = 0.8: Responsive, 20% weight to history
 * 
 * CUTOFF FREQUENCY CALCULATION:
 * For IIR filter with sampling rate Fs and alpha α:
 * fc = -Fs × ln(1-α) / (2π)
 * At 1 kHz and α=0.2: fc ≈ 35 Hz (good for human motion)
 * ============================================================================
 */

/* Filter type selection - uncomment ONE */
#define FILTER_TYPE_MOVING_AVG  (0)
#define FILTER_TYPE_IIR         (1)
#define FILTER_TYPE_KALMAN      (2)

#define ACTIVE_FILTER_TYPE      FILTER_TYPE_IIR

/* Moving Average Configuration */
#define MA_WINDOW_SIZE          (8U)    /* Number of samples to average */

/* IIR Filter Configuration
 * Alpha = 0.20 means:
 * Output = 0.20 × New_Sample + 0.80 × Previous_Output
 * This provides smooth tracking with ~35 Hz cutoff at 1 kHz sampling */
#define IIR_ALPHA               (0.20f)

/* ============================================================================
 * PWM CONFIGURATION
 * ============================================================================
 * PWM (Pulse Width Modulation) controls motor speed by varying
 * the duty cycle (percentage of time the signal is high).
 * 
 * FREQUENCY SELECTION:
 * - Too low (< 1 kHz): Audible whine, motor vibration
 * - Too high (> 20 kHz): Switching losses, EMI issues
 * - 10-20 kHz: Good compromise, above human hearing range
 * 
 * We'll use 10 kHz for balance of performance and efficiency.
 * 
 * DUTY CYCLE:
 * 0% = Motor stopped
 * 100% = Motor at full speed
 * Resolution: Determined by timer counter size
 * 
 * TIMER CALCULATION:
 * Timer clock = System clock / Prescaler
 * PWM period = (ARR + 1) / Timer_clock
 * 
 * For 10 kHz with 160 MHz system clock:
 * If Prescaler = 16: Timer_clock = 10 MHz
 * ARR = (10 MHz / 10 kHz) - 1 = 999
 * Resolution: 1000 steps (0.1% per step) - excellent
 * ============================================================================
 */
#define PWM_FREQUENCY_HZ        (10000U)
#define PWM_MIN_DUTY            (0U)        /* 0% duty cycle */
#define PWM_MAX_DUTY            (1000U)     /* 100% duty cycle (matches ARR) */

/* Timer clock calculations (assuming 160 MHz system clock) */
#define SYSTEM_CLOCK_HZ         (160000000U)
#define TIM_PRESCALER           (16U)       /* Divide by 16 → 10 MHz timer clock */
#define TIM_ARR_VALUE           ((SYSTEM_CLOCK_HZ / TIM_PRESCALER / PWM_FREQUENCY_HZ) - 1U)

/* RAMP CONFIGURATION
 * Smooth ramping prevents hydraulic shock.
 * RAMP_RATE defines how fast duty cycle changes per millisecond.
 * 10 units/ms = 1% per ms = 1000ms for 0-100% transition
 * This gives a smooth 1-second ramp-up time. */
#define RAMP_RATE_PER_MS        (10U)       /* Duty cycle units per millisecond */

/* ============================================================================
 * SAFETY THRESHOLDS
 * ============================================================================
 * Safety system monitors parameters and triggers FAULT state if exceeded.
 * These thresholds are derived from:
 * - Mechanical limits of the prosthetic
 * - Patient safety requirements
 * - Regulatory standards (ISO 13485, IEC 60601)
 * ============================================================================
 */

/* Communication timeout (ms) - no packet received = fault */
#define COMM_TIMEOUT_MS         (500U)

/* Watchdog timeout (ms) - task must refresh within this time */
#define SAFETY_WATCHDOG_MS      (100U)

/* ADC validity check - reading must be within this range */
#define ADC_MIN_RAW             (100U)      /* Below 100 = short to ground */
#define ADC_MAX_RAW             (4000U)     /* Above 4000 = open circuit */

/* Motor overcurrent protection (simulated via duty cycle limit) */
#define MOTOR_MAX_DUTY_FAULT    (950U)      /* 95% max before fault */

/* ============================================================================
 * STATE MACHINE DEFINITIONS
 * ============================================================================
 * Gait states represent different phases of movement.
 * State transitions are triggered by sensor patterns.
 * 
 * STATE IDs (0-16 as per requirement):
 * Compact uint8_t storage for Bluetooth transmission.
 * ============================================================================
 */
typedef enum {
    STATE_IDLE = 0,          /* System powered, no movement detected */
    STATE_STANDING = 1,      /* Upright, weight on leg */
    STATE_WALKING = 2,       /* Active gait cycle */
    STATE_SITTING = 3,       /* Knee flexed, weight off leg */
    STATE_STAIR_ASCENT = 4,  /* Climbing stairs */
    STATE_STAIR_DESCENT = 5, /* Descending stairs */
    STATE_RAMP_ASCENT = 6,   /* Walking up ramp */
    STATE_RAMP_DESCENT = 7,  /* Walking down ramp */
    STATE_TURNING = 8,       /* Pivoting on foot */
    STATE_BACKWARD = 9,      /* Walking backward */
    STATE_RUNNING = 10,      /* Jogging/running */
    STATE_CYCLING = 11,      /* Bicycle mode (if applicable) */
    STATE_CUSTOM_1 = 12,     /* Reserved for future use */
    STATE_CUSTOM_2 = 13,     /* Reserved for future use */
    STATE_CUSTOM_3 = 14,     /* Reserved for future use */
    STATE_CALIBRATION = 15,  /* Sensor calibration mode */
    STATE_FAULT = 16         /* Error state - safe mode */
} GaitState_t;

/* ============================================================================
 * PROTOCOL DEFINITIONS
 * ============================================================================
 * Packet structure constants for Bluetooth communication.
 * ============================================================================
 */
#define PROTOCOL_START_BYTE     (0xAAU)     /* Header identifier */
#define PROTOCOL_STOP_BYTE      (0x55U)     /* Trailer identifier (optional) */
#define PROTOCOL_PAYLOAD_LEN    (10U)       /* Fixed payload size in bytes */
#define PROTOCOL_PACKET_LEN     (12U)       /* Start + Len + Payload + Checksum */

/* Command codes for received packets */
typedef enum {
    CMD_NONE = 0x00,
    CMD_GET_STATUS = 0x01,
    CMD_START_CALIBRATION = 0x02,
    CMD_END_CALIBRATION = 0x03,
    CMD_SET_PARAMETER = 0x04,
    CMD_GET_PARAMETER = 0x05,
    CMD_RESET_FAULT = 0x06,
    CMD_ENTER_BOOTLOADER = 0x07,
    CMD_FACTORY_RESET = 0x08
} ProtocolCommand_t;

/* ============================================================================
 * FREERTOS CONFIGURATION
 * ============================================================================
 * Task priorities follow FreeRTOS convention:
 * Higher number = higher priority
 * Safety tasks must have highest priority for deterministic response.
 * 
 * STACK SIZE:
 * Specified in WORDS (4 bytes each on ARM Cortex-M).
 * 128 words = 512 bytes - minimum for simple tasks
 * 256 words = 1024 bytes - for tasks with local buffers
 * 512 words = 2048 bytes - for complex processing
 * ============================================================================
 */

/* Task Priorities (higher = more important) */
#define TASK_PRIORITY_SAFETY        (6U)    /* Highest - safety monitoring */
#define TASK_PRIORITY_MOTOR         (5U)    /* Motor control - time critical */
#define TASK_PRIORITY_ADC           (4U)    /* ADC processing */
#define TASK_PRIORITY_COMM          (3U)    /* USART2 communication */
#define TASK_PRIORITY_BLUETOOTH     (2U)    /* Telemetry - lower priority */
#define TASK_PRIORITY_WATCHDOG      (1U)    /* Lowest - watchdog refresh */

/* Task Periods (milliseconds) */
#define TASK_PERIOD_SAFETY_MS       (1U)    /* 1 kHz safety loop */
#define TASK_PERIOD_MOTOR_MS        (1U)    /* 1 kHz motor control */
#define TASK_PERIOD_ADC_MS          (5U)    /* 200 Hz ADC processing */
#define TASK_PERIOD_COMM_MS         (10U)   /* 100 Hz DMC communication */
#define TASK_PERIOD_BLUETOOTH_MS    (50U)   /* 20 Hz telemetry */
#define TASK_PERIOD_WATCHDOG_MS     (100U)  /* 10 Hz watchdog */

/* Task Stack Sizes (in words) */
#define TASK_STACK_SAFETY           (128U)
#define TASK_STACK_MOTOR            (256U)
#define TASK_STACK_ADC              (256U)
#define TASK_STACK_COMM             (256U)
#define TASK_STACK_BLUETOOTH        (512U)
#define TASK_STACK_WATCHDOG         (128U)

/* Queue Sizes */
#define QUEUE_UART_RX_SIZE          (16U)   /* Received packet queue */
#define QUEUE_COMMAND_SIZE          (8U)    /* Command queue */

/* ============================================================================
 * ERROR CODES
 * ============================================================================
 * Detailed error codes for diagnostics and logging.
 * Each code maps to a specific fault condition.
 * ============================================================================
 */
typedef enum {
    ERROR_NONE = 0x00,
    ERROR_ADC_TIMEOUT = 0x01,
    ERROR_ADC_OUT_OF_RANGE = 0x02,
    ERROR_ADC_DMA_FAILURE = 0x03,
    ERROR_UART_TX_FAILURE = 0x10,
    ERROR_UART_RX_FAILURE = 0x11,
    ERROR_UART_CRC_ERROR = 0x12,
    ERROR_UART_TIMEOUT = 0x13,
    ERROR_MOTOR_OVERCURRENT = 0x20,
    ERROR_MOTOR_STALL = 0x21,
    ERROR_MOTOR_DRIVER_FAULT = 0x22,
    ERROR_BATTERY_LOW = 0x30,
    ERROR_BATTERY_CRITICAL = 0x31,
    ERROR_COMM_TIMEOUT = 0x40,
    ERROR_COMM_PROTOCOL = 0x41,
    ERROR_WATCHDOG_RESET = 0x50,
    ERROR_STATE_MACHINE = 0x60,
    ERROR_CALIBRATION_FAILED = 0x70,
    ERROR_UNKNOWN = 0xFF
} ErrorCode_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
