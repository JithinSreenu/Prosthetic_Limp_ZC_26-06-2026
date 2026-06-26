Here are the two README files designed specifically for your project. You can save these as `README_SETUP_GUIDE.md` and `README_USER_MANUAL.md` in your project root folder.

***

# FILE 1: README_SETUP_GUIDE.md

# 📘 Prosthetic Knee Controller: Setup & Configuration Guide

## ⚠️ IMPORTANT: THE "OFFLINE WORKSTATION" RULE
Before setting up, you must understand this project's unique environment:
**Your main coding PC has no internet.** STM32CubeMX requires an active internet connection or a locally cached database to generate code properly. Because we manually wrote all initialization code (to avoid the classic SysTick conflict with FreeRTOS), **we DO NOT use CubeMX to write code.** We use it purely as a visual map to verify pinouts and clock speeds.

---

## 1. Hardware Requirements
*   **Microcontroller Board:** STMicroelectronics B-U585I-IOT02A Discovery Kit (STM32U585AI)
*   **Bluetooth Module:** HC-05 or similar UART Serial Profile (SPP) module
*   **Hydraulic Valve Motors:** Two DC motors with H-Bridge drivers
*   **Sensors:** 
    *   Load Cell (with analog amplifier outputting 0-3.3V)
    *   Knee Angle Sensor (analog 0-3.3V)
    *   Moment/Torque Sensor (analog 0-3.3V)
*   **Power:** USB-C via ST-LINK for debugging; External battery for motors (do not power motors from USB!).

---

## 2. Physical Wiring Map
Connect your external components to the Discovery board pins as follows:

| Peripheral | Pin on Board | Signal Type | Notes |
| :--- | :--- | :--- | :--- |
| **Load Cell Amp** | PA0 | Analog Input | ADC Channel 5 |
| **Angle Sensor** | PA1 | Analog Input | ADC Channel 6 |
| **Moment Sensor** | PA2 | Analog Input | ADC Channel 7 |
| **Battery Divider**| PA3 | Analog Input | ADC Channel 8. Use resistors to scale 5V down to 3.3V max! |
| **Bluetooth TX** | PA10 (USART1 RX)| UART RX | Cross-connected: BT TX goes to MCU RX |
| **Bluetooth RX** | PA9 (USART1 TX) | UART TX | Cross-connected: BT RX goes to MCU TX |
| **Bluetooth GND** | GND | Power | Common Ground is mandatory |
| **Motor A PWM** | PA8 | PWM Output | Timer 1 Channel 1 |
| **Motor A Dir** | PA7 | GPIO Output | To H-Bridge IN1 |
| **Motor B PWM** | PA15 | PWM Output | Timer 2 Channel 1 |
| **Motor B Dir** | PA6 | GPIO Output | To H-Bridge IN2 |
| **Status LED** | PE1 | GPIO Output | On-board LED (LD1) |

---

## 3. Step-by-Step STM32CubeMX Configuration
*Perform these steps on your Internet-Connected PC, save the `.ioc` file, and transfer it via USB to your Offline PC.*

### Step 3.1: Select MCU
1. Open CubeMX -> **Access to MCU Selector**
2. Search: `STM32U585AI`
3. Select `STM32U585AIIx` and click **Start Project**.

### Step 3.2: Clock Configuration (160 MHz)
1. Go to the **Clock Configuration** tab.
2. Set **HSE** to `Crystal/Ceramic Resonator`.
3. In the PLL1 configuration block:
   * **Input source:** HSE (16 MHz)
   * **Divider M:** `4`
   * **Multiplier N:** `80`
   * **System Clock Mux (P):** `2`
4. Verify the **HCLK** box turns green and reads `160 MHz`.
5. Set **AHB Prescaler** to `/1`, **APB1** to `/2`, **APB2** to `/2`.

### Step 3.3: Analog Setup (ADC1)
1. Go to **Pinout & Configuration** -> **Analog** -> **ADC1**.
2. Check **Continuous Conversion Mode** and **Scan Conversion Mode**.
3. Check **DMA Continuous Requests**.
4. Under *Rank* section, click `Add` 4 times and set:
   * Rank 1: Channel 5 (PA0)
   * Rank 2: Channel 6 (PA1)
   * Rank 3: Channel 7 (PA2)
   * Rank 4: Channel 8 (PA3)
5. Set all Sampling Times to `247.5 Cycles`.

### Step 3.4: Communication (USART1 & USART2)
1. Go to **Connectivity** -> **USART1**.
2. Mode: `Asynchronous`. Baud Rate: `115200`. Word Length: `8`. Parity: `None`. Stop Bits: `1`.
3. In the **DMA Settings** tab inside USART1:
   * Add `USART1_TX` (Memory to Peripheral, Normal)
   * Add `USART1_RX` (Peripheral to Memory, Normal)
4. Repeat the exact same steps for **USART2** (will map to PD5/PD6).

### Step 3.5: Motor PWM (TIM1 & TIM2)
1. Go to **Timers** -> **TIM1**.
2. Clock Source: `Internal Clock`.
3. Channel1: Check `PWM Generation CH1`.
4. Set **Prescaler (PSC)** to `15`. Set **Counter Period (ARR)** to `999`. *(This creates exactly 10 kHz PWM).*
5. Repeat for **TIM2**, enabling Channel1 with PSC=15 and ARR=999.

### Step 3.6: FreeRTOS (Visual Only)
1. Go to **Middleware** -> **FREERTOS**.
2. Interface: `CMSIS_V2`.
3. Kernel Settings: 
   * **USE_PREEMPTION:** Enabled
   * **TICK_RATE_HZ:** `1000`
   * **TOTAL_HEAP_SIZE:** `30720` (30 KB)
   * **Memory Management:** `heap_4`
   * **MAX_SYSCALL_INTERRUPT_PRIORITY:** `5` *(Crucial for DMA!)*

---

## 4. Sneakernet Integration (Offline PC)
1. Save the `.ioc` file on your Internet PC.
2. Move it to your Offline Working PC via USB drive.
3. On the Offline PC, place the `.ioc` file in your project's root folder.
4. **🛑 WARNING:** NEVER click "Generate Code" inside CubeMX on your offline PC. Doing so will overwrite our custom `HAL_InitTick` bypass and manual register configurations. Use the `.ioc` file *only* to visually inspect pinouts when troubleshooting hardware.

***

# FILE 2: README_USER_MANUAL.md

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
