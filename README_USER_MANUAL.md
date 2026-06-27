# README_USER_MANUAL.md

# 📗 Prosthetic Knee Controller: Beginner Manual & Customization Guide

Welcome to the firmware driving the Intelligent Prosthetic Knee. This manual explains how the code actually works under the hood, and serves as a step-by-step tutorial on how to modify the system's behavior.

---

## 1. The "Big Picture" (Architecture)
Imagine the firmware as a factory assembly line:
1. **The Sensors (ADC):** Constantly measuring force, angle, and torque in the background using DMA (Direct Memory Access). The CPU doesn't even lift a finger to do this; the hardware does it automatically.
2. **The Filters:** Taking raw, jittery sensor data and smoothing it out (like applying a blur filter in Photoshop, but for numbers).
3. **The Brain (State Machine):** Looking at the smooth data and deciding: *"Is the person standing? Walking? Sitting?"*
4. **The Muscles (Motor Control):** Taking the Brain's decision and telling the hydraulic valves to open or close smoothly using PWM.
5. **The Mouth (Bluetooth):** Shouting out the current status to a phone/PC 20 times a second so doctors can see what's happening.

---

## 2. The Golden Rule: No `HAL_Delay()`
In normal STM32 programs, you see `HAL_Delay(100);` to wait 100 milliseconds. **We never use this.** 
Because we are running FreeRTOS (a Real-Time Operating System), using `HAL_Delay()` freezes the whole chip. Instead, we use `vTaskDelay(pdMS_TO_TICKS(100));`. This tells the OS: *"I am taking a 100ms nap, wake me up then, but let other tasks run while I sleep."*

---

## 3. How to Change Things (Tutorial)

All the numbers that control how the knee behaves are located in one single file: **`App/Inc/app_config.h`**. You rarely need to touch the complex `.c` files. Just change a number here, recompile, and flash!

### 🛠️ Change A: Make the Bluetooth Telemetry Faster/Slower
By default, the system sends sensor data to your phone 20 times a second (every 50ms).
*   **Open:** `App/Inc/app_config.h`
*   **Find:** `#define TASK_PERIOD_BLUETOOTH_MS (50U)`
*   **To make it faster (50 Hz):** Change to `(20U)`
*   **To make it slower (10 Hz):** Change to `(100U)`
*   *Warning:* At 115200 Baud rate, sending a 13-byte packet takes about 0.1ms. Even at 100 Hz, you are only using 1% of the Bluetooth bandwidth. Don't go above 100 Hz without increasing the baud rate.

### 🛠️ Change B: Make the Sensor Data Smoother or More Responsive
We use an IIR Filter (Infinite Impulse Response). It uses a variable called `Alpha`.
*   **Open:** `App/Inc/app_config.h`
*   **Find:** `#define IIR_ALPHA (0.20f)`
*   **The Math:** `New Output = (0.20 * New Sensor Reading) + (0.80 * Old Output)`
*   **To make it smoother (ignores sudden spikes):** Lower the number. Try `0.10f`.
*   **To make it hyper-responsive (reacts instantly to bumps):** Raise the number. Try `0.50f`.
*   *Analogy:* Alpha is like the stiffness of a shock absorber on a car.

### 🛠️ Change C: Change How Fast the Valves Open/Close
When the knee decides to change valve position, it doesn't snap open instantly (that would cause a violent hydraulic jerk). It "ramps" up.
*   **Open:** `App/Inc/app_config.h`
*   **Find:** `#define RAMP_RATE_PER_MS (10U)`
*   **What it means:** Every 1 millisecond, the PWM duty cycle changes by 10 units. Since max duty is 1000, it takes exactly 100ms (0.1 seconds) to go from 0% to 100%.
*   **To make it slower/smoother:** Change to `(5U)` (takes 0.2 seconds to open).
*   **To make it snap faster:** Change to `(50U)` (takes 0.02 seconds to open).

### 🛠️ Change D: Adjust the PWM Motor Frequency
By default, the motors are driven at 10 kHz. This is above human hearing, so the motors won't "whine".
*   **Open:** `App/Inc/app_config.h`
*   **Find:** `#define PWM_FREQUENCY_HZ (10000U)`
*   **If you need higher frequency (e.g., for very specific high-speed motors):** Change to `20000U`. *(Note: The timer math will automatically recalculate the ARR value in the background).*

### 🛠️ Change E: Alter Safety Limits
What happens if the battery drops too low? Or if the load cell reads a crazy number?
*   **Open:** `App/Inc/app_config.h`
*   **Find the Battery Section:**
    ```c
    #define BATTERY_MIN_VALID_MV    (3000U)   /* System shut down at 3.0V */
    #define BATTERY_WARNING_MV      (3600U)   /* Warning at 3.6V */
    ```
*   **Change these** if your battery chemistry is different (e.g., if using a 3.7V LiPo, you might set warning to `3400U`).
*   **Find the Force Limits:**
    ```c
    #define FORCE_MIN_VALID         (-200)
    #define FORCE_MAX_VALID         (200)
    ```
    If you install a stronger load cell that reads up to 500N, change `FORCE_MAX_VALID` to `500`. If the system reads outside -200 to 200, it triggers a **FAULT** state and immediately kills the motors.

---

## 4. Understanding the Bluetooth Packet (Data Optimization)
To save mobile data and ensure fast transmission, we don't send text like `"Force: 150"`. We send raw binary bytes.

Look at **`App/Inc/protocol.h`**. A 13-byte packet looks like this in hex:
`AA 0A 00 96 00 50 0B B8 02 10 68 3F`
*   `AA` = Start marker (says "Hey, a packet is starting!")
*   `0A` = Length (10 bytes of payload)
*   `00 96` = Force (`0x0096` in hex = 150 in decimal)
*   `00 50` = Angle (80 degrees)
*   `0B B8` = Moment (`0x0BB8` = 3000 Nm)
*   `02` = State ID (2 = Walking)
*   `10 68` = Battery (`0x1068` = 4200 mV = 4.2V)
*   `3F` = Checksum (Sum of all middle bytes, used to detect if Bluetooth noise corrupted the data).

---

## 5. Folder Map (Where to find things)

*   📁 **`Core/Src/main.c`**: The boot sequence. Turns on clocks, starts the OS.
*   📁 **`Core/Src/stm32u5xx_it.c`**: The interrupt handlers. The "911 dispatchers" for hardware events. Notice `HAL_IncTick()` is intentionally missing from `SysTick_Handler`!
*   📁 **`App/Inc/app_config.h`**: **YOUR PLAYGROUND.** All tunable numbers live here.
*   📁 **`App/Src/adc_manager.c`**: The wizard that talks to the analog sensors.
*   📁 **`App/Src/filter.c`**: The math that smooths the sensor data.
*   📁 **`App/Src/state_machine.c`**: The logic that decides if the patient is walking or standing.
*   📁 **`App/Src/motor_control.c`**: The code that makes the hydraulic valves move.
*   📁 **`App/Src/uart_dma.c` & `bluetooth.c`**: The code that handles sending data to your phone without freezing the CPU.
*   📁 **`App/Src/safety.c`**: The bodyguard. If anything goes wrong, this file instantly cuts motor power.
*   📁 **`App/Src/freertos_tasks.c`**: The schedules for all the different workers (Tasks) in the factory.
