UART Generator
==============

Small command-line utility that replays semicolon-delimited UART telemetry files over a serial port.
It is useful for driving the ESP32 firmware with repeatable input while developing or testing the BLE and RemoteXY side.

What it sends
-------------

- Reads messages from `UART-Data_Produced-By-Nucleo_With-Emulated-Sensor-Input.txt`
- Splits the file on `;` and sends each non-empty package in sequence
- Appends a trailing `;` to each transmitted package
- Repeats the file in a loop at the requested interval

Requirements
------------

- Python 3.14 or newer
- Poetry
- `pyserial`

Setup
-----

- Install dependencies:

	poetry install

- Activate the virtual environment on PowerShell if you want to run the script directly:

	.\.venv\Scripts\Activate.ps1

Run
---

- Default settings:

	poetry run uart-generator

- From the activated virtual environment:

	uart-generator

- Custom serial port, baud rate, and data file:

	poetry run uart-generator --port COM6 --baudrate 115200 --data-file UART-Data_Produced-By-Nucleo_With-Emulated-Sensor-Input_scaled.txt

- Custom interval, for example 20 Hz:

	poetry run uart-generator --interval 0.05

- Add a line ending if your receiver expects one:

	poetry run uart-generator --newline "\n"

Options
-------

- `--port`: Serial port, default `COM6`
- `--baudrate`: Serial baud rate, default `115200`
- `--data-file`: Path to the semicolon-delimited input file
- `--interval`: Seconds between packages, default `1 / 70`
- `--newline`: Extra line ending appended after the trailing `;`

Notes
-----

- The script prints the first few outgoing payloads to stdout for quick verification.
- There are currently no automated tests in this folder.
