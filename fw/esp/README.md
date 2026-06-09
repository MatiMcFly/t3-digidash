# ESP32-C3 UART, BLE, and RemoteXY Bridge

This firmware receives ASCII telemetry over UART, parses it into car-signal values, and exposes those values over BLE.
It also embeds a RemoteXY-compatible BLE interface so a dashboard app can connect and read the same live data.

## What It Does
- Reads UART telemetry on GPIO4/GPIO5 at 115200 8N1
- Parses semicolon-separated `id:value` pairs into internal car-signal state
- Updates BLE characteristics for water temperature, RPM, battery voltage, tank level, turn signal, high beam, and oil-pressure switches
- Advertises a custom car-signals service plus standard Environmental Sensing and Battery services
- Exposes an HM-10-style BLE UART service for RemoteXY traffic
- Reconnects and re-advertises automatically after disconnects

## Current Firmware Layout
- `main/app_main.c` starts BLE, RemoteXY, and the UART task
- `main/uart_task.c` reads and parses UART telemetry
- `main/uart_parser.c` decodes the `id:value` message format
- `main/car_signals.c` stores signal state and triggers BLE notifications
- `main/gatt_db.c` defines the BLE GATT services and characteristics
- `main/remotexy_protocol.c` handles the RemoteXY command exchange

## Requirements
- ESP-IDF 6.0.1
- ESP32-C3 target
- A BLE client app such as nRF Connect, or a RemoteXY-compatible mobile app
- An external UART source that emits the expected telemetry format

## Build and Flash
```bash
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```

## Hardware Wiring
- ESP32-C3 GPIO4 (UART TX) -> RX of the external device
- ESP32-C3 GPIO5 (UART RX) -> TX of the external device
- GND -> GND
- RTS/CTS are not used

## UART Message Format
The parser expects a semicolon-separated stream of `id:value` pairs.

Example:
```text
1:23.5;2:1500;3:12.6;4:18.2;5:1;6:0;7:1;8:0;
```

Field mapping:
- `1` = water temperature
- `2` = RPM
- `3` = battery voltage
- `4` = tank level
- `5` = turn signal
- `6` = high beam
- `7` = oil pressure switch 3B
- `8` = oil pressure switch 18B

The parser is tolerant of streaming input and collects bytes until it finds `;` separators.

## BLE Services
The firmware advertises as `ESP32C3-NimBLE`.

Available services:
- Standard Environmental Sensing service `0x181A`
	- Temperature characteristic `0x2A6E`
- Standard Battery service `0x180F`
	- Battery Level characteristic `0x2A19`
- HM-10 compatible UART service `0xFFE0`
	- UART characteristic `0xFFE1` for RemoteXY command traffic
- Custom Car Signals service `c4e7b1a2-0f3d-4f2a-8d89-0d92b48e7d21`
	- RPM
	- Turn Signal
	- High Beam
	- Tank Level
	- Battery Voltage

Use nRF Connect to inspect the GATT table and enable notifications on the characteristics you want to watch.

## RemoteXY
RemoteXY support is built in and uses the BLE UART characteristic for control packets.
The firmware responds to configuration, ping, time, board, and variable update commands so a compatible dashboard can connect without extra glue code.

## UART Simulator
A small Python-based UART generator is available in `uart_py_simulator/` for local testing.
See that folder's README for setup and run commands.

## Notes
- BLE notifications are chunked for the UART/RemoteXY path
- Hardware flow control is disabled
- The project currently focuses on live telemetry bridging rather than local UI or storage
