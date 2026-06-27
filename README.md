Here are the two README files designed specifically for your project. You can save these as `README_SETUP_GUIDE.md` and `README_USER_MANUAL.md` in your project root folder.

***

# README_SETUP_GUIDE.md

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

