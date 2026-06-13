import argparse
import time

import serial


DEFAULT_DATA_FILE = "UART-Data_Produced-By-Nucleo_With-Emulated-Sensor-Input.txt"


def load_packages(file_path: str) -> list[str]:
    with open(file_path, "r", encoding="ascii") as handle:
        raw = handle.read()
    packages = [item.strip() for item in raw.split(";") if item.strip()]
    if not packages:
        raise ValueError(f"No packages found in {file_path}")
    return packages


def has_id_8(message: str) -> bool:
    return any(part.strip().startswith("8:") for part in message.split(";"))


def open_serial(port: str, baudrate: int) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1,
        write_timeout=1,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send UART messages from a semicolon-delimited data file."
    )
    parser.add_argument("--port", default="COM6", help="Serial port, e.g. COM6")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument(
        "--data-file",
        default=DEFAULT_DATA_FILE,
        help="Path to the semicolon-delimited data file.",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0 / 70.0,
        help="Seconds between packages (default 70 Hz).",
    )
    parser.add_argument("--newline", default="", help="Line ending to append.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    line_ending = args.newline

    packages = load_packages(args.data_file)
    package_index = 0
    sent_count = 0
    print("starting", flush=True)
    with open_serial(args.port, args.baudrate) as ser:
        while True:
            message = packages[package_index]
            package_index = (package_index + 1) % len(packages)
            payload = message + ";" + line_ending
            if sent_count < 20:
                print(payload, flush=True)
            sent_count += 1
            ser.write(payload.encode("ascii"))
            time.sleep(args.interval)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
