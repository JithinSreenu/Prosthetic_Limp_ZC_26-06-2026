[Prosthetic_Knee_Controller]/
│
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32u5xx_it.h
│   │   └── FreeRTOSConfig.h
│   └── Src/
│       ├── main.c
│       └── stm32u5xx_it.c
│
├── Drivers/
│   ├── CMSIS/
│   │   ├── Device/ST/STM32U5xx/Include/
│   │   └── Include/
│   └── STM32U5xx_HAL_Driver/
│       ├── Inc/
│       └── Src/
│
├── App/
│   ├── Inc/
│   │   ├── app_config.h
│   │   ├── adc_manager.h
│   │   ├── filter.h
│   │   ├── state_machine.h
│   │   ├── motor_control.h
│   │   ├── uart_dma.h
│   │   ├── protocol.h
│   │   ├── bluetooth.h
│   │   ├── safety.h
│   │   └── freertos_tasks.h
│   └── Src/
│       ├── adc_manager.c
│       ├── filter.c
│       ├── state_machine.c
│       ├── motor_control.c
│       ├── uart_dma.c
│       ├── protocol.c
│       ├── bluetooth.c
│       ├── safety.c
│       └── freertos_tasks.c
│
└── Third_Party/
    └── FreeRTOS/
        └── Source/
            ├── include/
            ├── tasks.c
            ├── queue.c
            ├── list.c
            ├── timers.c
            ├── event_groups.c
            ├── stream_buffer.c
            └── portable/
                ├── GCC/
                │   └── ARM_CM33_NTZ/
                │       ├── port.c
                │       └── portmacro.h
                └── MemMang/
                    └── heap_4.c
