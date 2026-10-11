DIY Royal Enfield Tripper Digital Gauge
A DIY experiment replacing the internals of a Royal Enfield Tripper gauge with a Waveshare ESP32-S3 1.75-inch round AMOLED display and an ELM327 OBD-II adapter.
The goal is to display motorcycle data using a custom UI inspired by Garage Tinkering's minimal speedometer project.
[!WARNING]
Experimental project — use at your own risk. This project is shared for informational purposes only. I make no guarantees about its safety, reliability, or suitability for road use, and I accept no responsibility for damage, injury, or other issues resulting from attempting this modification. You are responsible for checking the electrical connections, compatibility, and local requirements before use.

Components
- 2-pin SH 1.0 mm male connector
- OBD2-to-Honda 6-pin adapter
- Mini automotive fuse tap
- 12 V to 3.3 V buck converter
- Round plexiglass disc (approximately 6 cm in diameter and 1.5 mm thick)
- Waveshare ESP32-S3 1.75-inch round AMOLED display with case
- ELM327 adapter (mine uses a PIC18F25K80, with a Bluetooth module soldered on top)

How it works
The Bluetooth module is desoldered from the ELM327 board, and wires are connected to its TX/RX pins instead. The ELM327 is then connected to the OBD2-to-Honda adapter and placed under the seat.
The ESP32 communicates with the ELM327 over UART. Power is supplied by a buck converter, with its input fed through an automotive fuse tap.

UI attribution
The gauge UI design is based on work by Garage Tinkering. See the original GitHub repository.
https://github.com/garagetinkering/minimal-speedo
