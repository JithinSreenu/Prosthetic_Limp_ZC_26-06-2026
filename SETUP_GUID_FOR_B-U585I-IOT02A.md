# 📘 Prosthetic Knee Controller: Complete Setup Guide
## For B-U585I-IOT02A Discovery Kit (STM32U585AI)

---

## ⚠️ CRITICAL: READ THIS FIRST

### The "Offline Workstation" Rule
Your main coding PC has **NO INTERNET**. STM32CubeMX requires internet to generate code. We **manually wrote all initialization code** (to avoid SysTick conflicts with FreeRTOS). We use CubeMX **ONLY** as a visual pinout reference.

### The Golden Rule
```
🛑 NEVER click "Generate Code" on your offline PC.
   It will DESTROY our custom HAL_InitTick bypass.
```

---

## 1. Hardware Requirements

### 1.1 Main Board
| Component | Specification |
|-----------|---------------|
| **Board** | STMicroelectronics B-U585I-IOT02A Discovery Kit |
| **MCU** | STM32U585AIIxQ |
| **Package** | UFQFPN48 |
| **Flash** | 2 MB |
| **RAM** | 784 KB SRAM |
| **Max Clock** | 160 MHz |

### 1.2 External Components
| Peripheral | Specification | Qty |
|------------|---------------|-----|
| Bluetooth Module | HC-05 or HC-06 (SPP, 3.3V compatible) | 1 |
| H-Bridge Driver | TB6612FNG or DRV8833 (for 2 DC motors) | 1 |
| DC Motors | Hydraulic valve actuators | 2 |
| Load Cell Amplifier | HX711 or similar (0-3.3V analog output) | 1 |
| Knee Angle Sensor | Potentiometer/encoder (0-3.3V analog output) | 1 |
| Moment/Torque Sensor | Strain gauge amp (0-3.3V analog output) | 1 |
| Voltage Divider Resistors | 10kΩ + 6.8kΩ (for 5V→3.3V scaling) | 2 |
| External Battery | 7.4V-11.1V LiPo (for motors ONLY) | 1 |
| USB-C Cable | For ST-LINK power/debug | 1 |

---

## 2. B-U585I-IOT02A Pinout Map

### 2.1 Complete Wiring Diagram

```
                    B-U585I-IOT02A DISCOVERY BOARD
                    ┌─────────────────────────────┐
                    │                             │
    LOAD CELL ──────┤ PA0  (ADC1_IN5)             │
    ANGLE SENSOR ───┤ PA1  (ADC1_IN6)             │
    MOMENT SENSOR ──┤ PA2  (ADC1_IN7)             │
    BATT DIVIDER ───┤ PA3  (ADC1_IN8)             │
                    │                             │
    BT RX ──────────┤ PA9  (USART1_TX)  ──────────┼──→ TO BT RX
    BT TX ──────────┤ PA10 (USART1_RX)  ──────────┼──→ FROM BT TX
    BT GND ─────────┤ GND              ──────────┼──→ TO BT GND
                    │                             │
    MOTOR A PWM ────┤ PA8  (TIM1_CH1)             │
    MOTOR A DIR ────┤ PA7  (GPIO)                 │
    MOTOR B PWM ────┤ PA15 (TIM2_CH1)             │
    MOTOR B DIR ────┤ PA6  (GPIO)                 │
                    │                             │
    STATUS LED ─────┤ PE1  (LD1 - On-board)       │
                    │                             │
                    │  USB-C (ST-LINK)             │
                    │  [DEBUG POWER ONLY]          │
                    └─────────────────────────────┘
```

### 2.2 Detailed Pin Table

| Function | Board Pin | STM32 Pin | Alternate Function | Signal Type | Notes |
|----------|-----------|-----------|-------------------|-------------|-------|
| Load Cell | CN7 Pin 5 | PA0 | ADC1_IN5 | Analog In | 0-3.3V from amplifier |
| Angle Sensor | CN7 Pin 6 | PA1 | ADC1_IN6 | Analog In | 0-3.3V |
| Moment Sensor | CN7 Pin 7 | PA2 | ADC1_IN7 | Analog In | 0-3.3V |
| Battery Divider | CN7 Pin 8 | PA3 | ADC1_IN8 | Analog In | See §2.3 |
| Bluetooth RX | CN9 Pin 1 | PA9 | USART1_TX | UART TX | Cross-connect |
| Bluetooth TX | CN9 Pin 2 | PA10 | USART1_RX | UART RX | Cross-connect |
| Motor A PWM | CN9 Pin 3 | PA8 | TIM1_CH1 | PWM Out | 10 kHz default |
| Motor A Direction | CN9 Pin 5 | PA7 | GPIO | Digital Out | To H-Bridge IN1 |
| Motor B PWM | CN9 Pin 4 | PA15 | TIM2_CH1 | PWM Out | 10 kHz default |
| Motor B Direction | CN9 Pin 6 | PA6 | GPIO | Digital Out | To H-Bridge IN2 |
| Status LED | On-board | PE1 | GPIO | Digital Out | LD1 (Green) |
| Ground | CN7/9 GND | GND | - | Power | **COMMON GND REQUIRED** |

### 2.3 Battery Voltage Divider Circuit

```
                    BATTERY (+)
                         │
                        [R1] 10kΩ
                         │
                         ├─────────── TO PA3 (ADC)
                         │
                        [R2] 6.8kΩ
                         │
                        GND
                         │
                    BATTERY (-)

Calculation:
Vout = Vbat × R2/(R1+R2) = Vbat × 6.8/16.8 = Vbat × 0.405

For 8.4V (2S LiPo full):  Vout = 3.40V ✓ (Safe!)
For 6.0V (2S LiPo empty): Vout = 2.43V ✓
For 12.6V (3S LiPo full): Vout = 5.10V ✗ (TOO HIGH!)

⚠️ If using 3S battery, use R1=20kΩ, R2=6.8kΩ instead
```

---

## 3. STM32CubeMX Configuration (Internet PC Only)

### 3.1 Create New Project

```
1. Open STM32CubeMX
2. File → New Project
3. Click "Access to MCU Selector"
4. Search: "STM32U585AI"
5. Select: STM32U585AIIxQ (Package UFQFPN48)
6. Click "Start Project"
```

### 3.2 Clock Configuration (160 MHz)

Navigate to **Clock Configuration** tab:

```
┌─────────────────────────────────────────────────────────────────┐
│                    PLL1 CONFIGURATION                            │
├─────────────────────────────────────────────────────────────────┤
│  Input Source: [HSE 16 MHz Crystal/Ceramic Resonator]           │
│                                                                  │
│  Divider M:  [4]                                                 │
│  Multiplier N: [80]                                              │
│  System Clock Mux (P): [2]                                       │
│                                                                  │
│  Result: 16 / 4 × 80 / 2 = 160 MHz ✓                            │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    BUS PRESCALERS                                │
├─────────────────────────────────────────────────────────────────┤
│  HCLK (CPU Clock):     [/1] → 160 MHz                           │
│  APB1 Prescaler:       [/2] → 80 MHz                            │
│  APB2 Prescaler:       [/2] → 80 MHz                            │
│  APB3 Prescaler:       [/2] → 80 MHz                            │
└─────────────────────────────────────────────────────────────────┘

⚠️ HCLK box MUST turn GREEN. If yellow/red, check HSE is selected.
```

### 3.3 ADC1 Configuration (4-Channel DMA)

Navigate to **Pinout & Configuration → Analog → ADC1**:

```
┌─────────────────────────────────────────────────────────────────┐
│  ADC1 Settings                                                   │
├─────────────────────────────────────────────────────────────────┤
│  ☑ Continuous Conversion Mode                                    │
│  ☑ Scan Conversion Mode                                         │
│  ☑ DMA Continuous Requests                                      │
│                                                                  │
│  Clock Prescaler: PCLK2/4                                       │
│  Resolution: 12 bits (0-4095)                                   │
│  Data Alignment: Right                                          │
│  Scan Direction: Upward                                         │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  Rank Configuration (Click "Add" 4 times)                        │
├─────────────────────────────────────────────────────────────────┤
│  Rank 1: Channel 5  (PA0 - Load Cell)    Sampling: 247.5 Cycles │
│  Rank 2: Channel 6  (PA1 - Angle)        Sampling: 247.5 Cycles │
│  Rank 3: Channel 7  (PA2 - Moment)       Sampling: 247.5 Cycles │
│  Rank 4: Channel 8  (PA3 - Battery)      Sampling: 247.5 Cycles │
└─────────────────────────────────────────────────────────────────┘
```

### 3.4 USART1 Configuration (Bluetooth)

Navigate to **Pinout & Configuration → Connectivity → USART1**:

```
┌─────────────────────────────────────────────────────────────────┐
│  USART1 Mode                                                     │
├─────────────────────────────────────────────────────────────────┤
│  Mode: Asynchronous                                              │
│                                                                  │
│  Baud Rate:     115200                                           │
│  Word Length:   8 Bits                                           │
│  Parity:        None                                             │
│  Stop Bits:     1                                                │
│  Data Direction: Receive and Transmit                            │
│  Over Sampling: 16 Samples                                       │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  USART1 DMA Settings (Click "Add" for each)                      │
├─────────────────────────────────────────────────────────────────┤
│  USART1_TX:                                                      │
│    Direction: Memory to Peripheral                               │
│    Mode: Normal                                                  │
│    Data Width: Byte                                             │
│    Priority: Medium                                             │
│                                                                  │
│  USART1_RX:                                                      │
│    Direction: Peripheral to Memory                               │
│    Mode: Normal                                                  │
│    Data Width: Byte                                             │
│    Priority: High (RX is more important!)                       │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  USART1 NVIC Settings (Interrupts)                               │
├─────────────────────────────────────────────────────────────────┤
│  ☑ USART1 global interrupt    Priority: 6                       │
│  ☑ DMA1 channel1 global interrupt  Priority: 6                  │
│  ☑ DMA1 channel2 global interrupt  Priority: 6                  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.5 TIM1 Configuration (Motor A PWM)

Navigate to **Pinout & Configuration → Timers → TIM1**:

```
┌─────────────────────────────────────────────────────────────────┐
│  TIM1 Mode                                                       │
├─────────────────────────────────────────────────────────────────┤
│  Clock Source: Internal Clock                                    │
│  Channel1: ☑ PWM Generation CH1                                 │
│                                                                  │
│  Counter Settings:                                               │
│    Prescaler (PSC):     15                                       │
│    Counter Mode:        Up                                       │
│    Counter Period (ARR): 999                                     │
│    Internal Clock Division: No Division                          │
│    auto-reload preload: Enable                                   │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  TIM1 CH1 PWM Settings                                           │
├─────────────────────────────────────────────────────────────────┤
│  Mode: PWM mode 1                                                │
│  Pulse (CCR1): 0                                                 │
│  Output compare preload: Enable                                  │
│  Fast Mode: Disable                                              │
│  CH Polarity: High                                               │
└─────────────────────────────────────────────────────────────────┘

PWM Frequency Calculation:
F_pwm = 160MHz / (PSC+1) / (ARR+1)
F_pwm = 160,000,000 / 16 / 1000 = 10,000 Hz = 10 kHz ✓
```

### 3.6 TIM2 Configuration (Motor B PWM)

Navigate to **Pinout & Configuration → Timers → TIM2**:

```
Same settings as TIM1:
  ☑ PWM Generation CH1
  Prescaler (PSC): 15
  Counter Period (ARR): 999
  All other settings: IDENTICAL to TIM1
```

### 3.7 GPIO Configuration (Motor Direction & LED)

Navigate to **Pinout & Configuration → System Core → GPIO**:

```
┌─────────────────────────────────────────────────────────────────┐
│  GPIO Pin Configuration                                          │
├─────────────┬───────────────┬───────────────┬───────────────────┤
│    Pin      │   GPIO Mode   │  Pull-up/down │   Label           │
├─────────────┼───────────────┼───────────────┼───────────────────┤
│    PA6      │ Output Push   │ No pull       │ MOTOR_B_DIR       │
│    PA7      │ Output Push   │ No pull       │ MOTOR_A_DIR       │
│    PE1      │ Output Push   │ No pull       │ STATUS_LED        │
└─────────────┴───────────────┴───────────────┴───────────────────┘

Initial Output Level for all: Low (0)
GPIO Speed: Low (direction pins don't need to be fast)
```

### 3.8 FreeRTOS Configuration (VISUAL REFERENCE ONLY)

Navigate to **Pinout & Configuration → Middleware → FREERTOS**:

```
┌─────────────────────────────────────────────────────────────────┐
│  FREERTOS Interface: CMSIS_V2                                   │
├─────────────────────────────────────────────────────────────────┤
│  KERNEL SETTINGS                                                 │
│  ├─ USE_PREEMPTION: Enabled                                     │
│  ├─ TICK_RATE_HZ: 1000                                          │
│  ├─ TOTAL_HEAP_SIZE: 30720 (30 KB)                              │
│  ├─ Memory Management: heap_4                                   │
│  └─ MAX_SYSCALL_INTERRUPT_PRIORITY: 5  ← CRITICAL FOR DMA!      │
│                                                                  │
│  CONFIG PARAMETERS                                               │
│  ├─ USE_MUTEXES: Enabled                                        │
│  ├─ USE_RECURSIVE_MUTEXES: Disabled                             │
│  └─ USE_COUNTING_SEMAPHORES: Disabled                           │
└─────────────────────────────────────────────────────────────────┘

⚠️ This is for VISUAL REFERENCE ONLY.
   Our code configures FreeRTOS manually!
```

---

## 4. Sneakernet Transfer (Internet → Offline PC)

### 4.1 Transfer Procedure

```
┌──────────────────────┐         USB Drive         ┌──────────────────────┐
│   INTERNET PC        │  ───────────────────────→  │   OFFLINE PC         │
│                      │                            │                      │
│   • Generate .ioc    │                            │   • Place .ioc in    │
│   • Save to USB      │                            │     project root     │
│                      │                            │   • Open for VIEWING │
└──────────────────────┘                            └──────────────────────┘
```

### 4.2 What to Transfer

```
📁 USB Drive
└── 📁 Prosthetic_Knee_Project
    ├── 📄 prosthetic_knee.ioc    ← CUBEMX FILE (for reference only)
    └── 📁 Full_Source_Code        ← Your actual working code
```

### 4.3 ⛔ WHAT NEVER TO DO ON OFFLINE PC

```
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║   🚫 DO NOT click "Generate Code" in CubeMX                     ║
║   🚫 DO NOT click "Open Project" then Generate                  ║
║   🚫 DO NOT use CubeMX to modify any .c or .h files             ║
║                                                                  ║
║   REASON: It will overwrite our custom SysTick handler that     ║
║   bypasses HAL_InitTick() for FreeRTOS compatibility.           ║
║                                                                  ║
║   ✅ ONLY use .ioc file to VISUALLY verify pinouts              ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 5. Debug Configuration (STM32CubeIDE)

### 5.1 Debug Probe Settings

```
Run → Debug Configurations → STM32 C/C++ Application

┌─────────────────────────────────────────────────────────────────┐
│  Debug Probe Settings                                            │
├─────────────────────────────────────────────────────────────────┤
│  Probe: ST-LINK (ST-LINK_V3 embedded on B-U585I-IOT02A)        │
│  Interface: SWD                                                 │
│  Frequency: 4 MHz (default)                                     │
│  Mode: Debug                                                    │
│                                                                  │
│  ☑ Connect under reset                                          │
│  ☐ Enable external flash loader                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Flash Settings

```
┌─────────────────────────────────────────────────────────────────┐
│  Flash Download Settings                                         │
├─────────────────────────────────────────────────────────────────┤
│  Programming Algorithm: STM32U5xx 2MB Flash                     │
│  Start Address: 0x08000000                                      │
│  Size: 0x200000 (2 MB)                                          │
│                                                                  │
│  ☑ Reset and Run (auto-start after flash)                       │
│  ☑ Verify                                                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Power Considerations

### 6.1 Power Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     POWER DISTRIBUTION                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   USB-C (ST-LINK)          External Battery (7.4V-11.1V)        │
│        │                            │                             │
│        ▼                            ▼                             │
│   ┌─────────┐                  ┌─────────┐                       │
│   │ 5V USB  │                  │ Motor   │                       │
│   │ → LDO   │                  │ Power   │                       │
│   │ → 3.3V  │                  │ Supply  │                       │
│   └────┬────┘                  └────┬────┘                       │
│        │                            │                             │
│        ▼                            ▼                             │
│   ┌─────────┐                  ┌─────────┐                       │
│   │  STM32  │                  │H-Bridge │                       │
│   │  + BT   │                  │→ Motors │                       │
│   │ + Sensors│                 │         │                       │
│   └─────────┘                  └─────────┘                       │
│                                                                  │
│   ⚠️ DO NOT connect motor power to USB!                         │
│   ⚠️ Common GND between USB power and battery is REQUIRED!      │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Current Budget

| Component | Current Draw | Source |
|-----------|--------------|--------|
| STM32U585 (active) | ~15 mA | USB |
| HC-05 Bluetooth | ~30 mA (TX), ~8 mA (idle) | USB |
| Load Cell Amplifier | ~5 mA | USB |
| Sensors (3x analog) | ~15 mA total | USB |
| **Total MCU Side** | **~73 mA** | **USB (Safe)** |
| Motor A (stall) | ~1.5 A | Battery |
| Motor B (stall) | ~1.5 A | Battery |
| H-Bridge Logic | ~10 mA | Battery |
| **Total Motor Side** | **~3 A peak** | **Battery** |

---

## 7. Verification Checklist

### 7.1 Hardware Verification (Before Power On)

```
☐ All GND connections are common (multimeter beep test)
☐ Battery divider outputs < 3.3V at max battery voltage
☐ H-Bridge motor power NOT connected to USB 5V
☐ HC-05 VCC connected to 3.3V (NOT 5V - some HC-05 are 5V only!)
☐ No shorts between adjacent pins (especially PA8/PA9/PA10)
☐ Load cell amplifier output range is 0-3.3V
```

### 7.2 Software Verification (First Flash)

```
☐ Status LED (PE1) blinks on boot
☐ Bluetooth module shows steady LED (paired) or fast blink (waiting)
☐ Motor PWM outputs show ~10 kHz on oscilloscope (PA8, PA15)
☐ ADC readings change when sensors are moved
☐ No HardFault on startup
```

### 7.3 Functional Verification

```
☐ Bluetooth packet received on phone/PC (look for 0xAA start byte)
☐ State machine changes from IDLE to STANDING when load detected
☐ Motors respond to state changes (direction pins toggle)
☐ PWM ramp is visible (gradual speed change, not instant)
☐ Safety fault triggers when sensor is disconnected (reads 0 or 4095)
```

---

## 8. Troubleshooting Guide

### 8.1 Common Issues

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| No LED blink | Clock not configured | Check HSE is 16MHz crystal, not bypass |
| HardFault on boot | SysTick conflict | Verify `HAL_IncTick()` is NOT in `SysTick_Handler` |
| ADC reads all zeros | DMA not started | Check `ADC1` DMA is enabled in code |
| ADC reads 4095 constantly | Floating input | Check sensor GND connection |
| Bluetooth garbage | Baud mismatch | Verify 115200 on both sides |
| Motors don't move | H-Bridge no power | Check external battery connection |
| Motors twitch randomly | No common GND | Connect battery GND to MCU GND |
| FreeRTOS crashes | Interrupt priority > 5 | DMA/UART IRQ must be ≥ 5 |

### 8.2 Debug Print via SWO

If Bluetooth isn't working, use SWO for debug output:

```c
// In your code, you can add SWO debug if needed
// (Requires ITM viewer in CubeIDE)
ITM_SendChar('A');  // Single character
```

---

## 9. Quick Reference Card

### Pin Summary (Print This)

```
┌─────────────────────────────────────────────────┐
│         B-U585I-IOT02A PIN MAP                  │
├──────────────┬──────────────┬───────────────────┤
│     PIN      │  FUNCTION    │     CONNECT TO    │
├──────────────┼──────────────┼───────────────────┤
│     PA0      │  ADC1_IN5    │  Load Cell Amp    │
│     PA1      │  ADC1_IN6    │  Angle Sensor     │
│     PA2      │  ADC1_IN7    │  Moment Sensor    │
│     PA3      │  ADC1_IN8    │  Battery Divider  │
│     PA6      │  GPIO OUT    │  Motor B Dir      │
│     PA7      │  GPIO OUT    │  Motor A Dir      │
│     PA8      │  TIM1_CH1    │  Motor A PWM      │
│     PA9      │  USART1_TX   │  BT RX            │
│     PA10     │  USART1_RX   │  BT TX            │
│     PA15     │  TIM2_CH1    │  Motor B PWM      │
│     PE1      │  GPIO OUT    │  Status LED       │
│     GND      │  POWER       │  Common Ground    │
└──────────────┴──────────────┴───────────────────┘
```

### Clock Summary

```
HSE: 16 MHz (Crystal)
PLL1: 16/4 × 80/2 = 160 MHz
HCLK: 160 MHz
APB1: 80 MHz (TIM2 clock = 160 MHz)
APB2: 80 MHz (TIM1 clock = 160 MHz)
PWM: 160MHz/16/1000 = 10 kHz
```

### Key Configuration Values

```
ADC: 4 channels, 12-bit, DMA continuous
UART1: 115200 baud, 8N1, DMA TX/RX
TIM1: PWM CH1, PSC=15, ARR=999
TIM2: PWM CH1, PSC=15, ARR=999
FreeRTOS: 1000 Hz tick, 30KB heap
Max SysCall Priority: 5
```

---

*Document Version: 1.0*
*Board: B-U585I-IOT02A*
*MCU: STM32U585AIIxQ*
*Date: 2024*
