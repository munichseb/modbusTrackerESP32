Required Hardware:

- ESP32 development board: For example an ESP32 DevKit V1.
- RS485 transceiver module: For example, a MAX485 module.
- Connection cable and plug connector: For connection to the MODBUS RS485 bus.

Hardware connections:
1. connection of the RS485 module to the ESP32:

MAX485:
RO (Receiver Output) ➔ RX-Pin des ESP32 (z. B. GPIO16).
DI (Driver Input) ➔ TX-Pin des ESP32 (z. B. GPIO17).
DE (Driver Enable) und RE (Receiver Enable) zusammen ➔ GPIO4 des ESP32.
Hinweis: Durch Ansteuern von DE und RE können wir zwischen Senden und Empfangen wechseln. Für das Mithören setzen wir beide auf LOW (Empfangsmodus).
VCC ➔ 3.3V des ESP32.
GND ➔ GND des ESP32.

2. connection to the MODBUS RS485 bus:

Connect A and B of the MAX485 module in parallel to the A and B lines of the existing RS485 bus.
Attention: The polarity must be correct. A to A, B to B.
Important: Make sure that your connection does not interfere with the bus. If possible, use a passive eavesdropping method.
