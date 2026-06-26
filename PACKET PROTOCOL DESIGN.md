╔═══════════════════════════════════════════════════════════════════════════════╗
║                        BLUETOOTH PACKET STRUCTURE                             ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║  BYTE OFFSET:  0    1    2    3    4    5    6    7    8    9   10   11   12  ║
║┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐║
║│ 0xAA│ LEN │SP AN│FORCE│FORCE│KN AN│KN AN│MOMNT│MOMNT│STAT │BAT V│BAT V│ CHK │║
║│START│ 10  │     │  H  │  L  │  H  │  L  │  H  │  L  │     │  H  │  L  │SUM  │║
║└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘║
║                                                                               ║
║  TOTAL PACKET SIZE: 13 BYTES                                                  ║
║                                                                               ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║  FIELD DETAILS:                                                               ║
║                                                                               ║
║  START BYTE (0xAA): Fixed header identifier                                   ║
║                   10101010 binary - distinctive pattern                       ║
║                   Not a valid data value - easy detection                     ║
║                                                                               ║
║  LENGTH (10): Fixed payload length                                            ║
║               Allows receiver to validate packet integrity                    ║
║               Receiver can skip corrupted packets by counting bytes           ║
║                                                                               ║
║  SPOOL ANGLE (uint8_t): 0-100 representing valve position                     ║
║                          Direct value, no scaling needed                      ║
║                                                                               ║
║  FORCE (int16_t, Big-Endian): ±200 Newtons                                    ║
║          Byte 3 = High byte (sign + upper bits)                               ║
║          Byte 4 = Low byte (lower bits)                                       ║
║          Example: +150 = 0x0096 → Bytes: 0x00, 0x96                           ║
║          Example: -50  = 0xFFCE → Bytes: 0xFF, 0xCE                           ║
║                                                                               ║
║  KNEE ANGLE (int16_t, Big-Endian): ±200 degrees                               ║
║            Same encoding as Force                                             ║
║            0 = fully extended, -200 = fully flexed                            ║
║                                                                               ║
║  MOMENT (int16_t, Big-Endian): ±3000 Nm                                       ║
║          Same encoding as Force                                               ║
║          Torque around knee joint                                             ║
║                                                                               ║
║  STATE ID (uint8_t): 0-16 representing gait phase                             ║
║          0=IDLE, 1=STANDING, 2=WALKING, 3=SITTING,                            ║
║          4=STAIR_ASCENT, 5=STAIR_DESCENT, 16=CALIBRATION,                     ║
║          15=FAULT                                                             ║
║                                                                               ║
║  BATTERY (uint16_t, Big-Endian): 0-5000 millivolts                            ║
║          Byte 10 = High byte                                                  ║
║          Byte 11 = Low byte                                                   ║
║          Example: 4.2V = 4200mV = 0x1068 → Bytes: 0x10, 0x68                  ║
║                                                                               ║
║  CHECKSUM (uint8_t): Simple additive checksum                                 ║
║          Calculated: (LEN + SP_AN + FORCE_H + FORCE_L + ... + BAT_L)          ║
║          Receiver: Sum all bytes from LEN to BAT_L, compare to CHK            ║
║          If mismatch → packet discarded                                       ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
