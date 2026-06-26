╔═══════════════════════════════════════════════════════════════════════════════╗
║                      TELEMETRY DATA OPTIMIZATION TABLE                        ║
╠═════════════════════╦═══════════════╦════════════════╦═══════════════════════╣
║     VARIABLE        ║  RAW RANGE    ║ OPTIMIZED TYPE ║    MEMORY SAVED       ║
╠═════════════════════╬═══════════════╬════════════════╬═══════════════════════╣
║ Spool Angle         ║  0 to 100     ║  uint8_t       ║  3 bytes (vs float)   ║
║ Force Reading       ║  ±200         ║  int16_t       ║  2 bytes (vs float)   ║
║ Knee Angle          ║  ±200         ║  int16_t       ║  2 bytes (vs float)   ║
║ Moment Reading      ║  ±3000        ║  int16_t       ║  2 bytes (vs float)   ║
║ State ID            ║  0 to 16      ║  uint8_t       ║  3 bytes (vs float)   ║
║ Battery Monitor     ║  0 to 5V      ║  uint16_t (mV) ║  2 bytes (vs float)   ║
╠═════════════════════╬═══════════════╬════════════════╬═══════════════════════╣
║ TOTAL PAYLOAD       ║               ║  10 bytes      ║  14 bytes saved       ║
╚═════════════════════╩═══════════════╩════════════════╩═══════════════════════╝

MEMORY COMPARISON:
┌─────────────────────────────────────────────────────────────────┐
│  Using float (4 bytes each): 6 × 4 = 24 bytes payload          │
│  Using optimized types:     10 bytes payload                    │
│                                                                 │
│  Memory Saved: 14 bytes per packet                              │
│  At 20 Hz: 280 bytes/second saved                              │
│  At 50 Hz: 700 bytes/second saved                              │
└─────────────────────────────────────────────────────────────────┘
