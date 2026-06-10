#!/usr/bin/env python3
"""
Live Motor RPM monitor over serial.

Connects to a serial device, parses incoming datasets and plots
Motor RPM on a continuously updating time axis.

Usage:
    python3 live_motor_rpm.py                          # /dev/ttyACM0, 115200 baud
    python3 live_motor_rpm.py --port /dev/ttyUSB0
    python3 live_motor_rpm.py --port /dev/ttyACM0 --baud 9600 --window 120
"""

import argparse
import queue
import re
import threading
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial


# ---------------------------------------------------------------------------
# Serial reader thread
# ---------------------------------------------------------------------------

def serial_reader(port: str, baud: int, data_queue: queue.Queue, stop_event: threading.Event) -> None:
    """
    Open the serial port and parse incoming lines.
    Each complete dataset (bounded by 'Turn Signal:' lines) is reduced to a
    single (timestamp, rpm) tuple and pushed onto data_queue.
    Missing 'Motor RPM:' within a dataset is treated as 0.
    """
    rpm_pattern = re.compile(r"Motor RPM:\s+(\d+)\s+rpm")
    current_rpm: int = 0
    in_dataset: bool = False

    print(f"Opening {port} at {baud} baud …")
    try:
        ser = serial.Serial(port, baud, timeout=1)
    except serial.SerialException as exc:
        print(f"ERROR: cannot open serial port: {exc}")
        stop_event.set()
        return

    print("Connected. Waiting for data …")

    try:
        while not stop_event.is_set():
            try:
                raw = ser.readline()
            except serial.SerialException as exc:
                print(f"Serial read error: {exc}")
                break

            if not raw:
                continue

            try:
                line = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue

            if line.startswith("Turn Signal:"):
                if in_dataset:
                    # Completed dataset → emit
                    data_queue.put((time.monotonic(), current_rpm))
                # Start new dataset
                current_rpm = 0
                in_dataset = True
                continue

            if in_dataset:
                m = rpm_pattern.match(line)
                if m:
                    current_rpm = int(m.group(1))
    finally:
        ser.close()
        print("Serial port closed.")


# ---------------------------------------------------------------------------
# Live plot
# ---------------------------------------------------------------------------

def run_live_plot(port: str, baud: int, window: float) -> None:
    """Set up a live scrolling RPM plot."""
    data_queue: queue.Queue = queue.Queue()
    stop_event = threading.Event()

    reader_thread = threading.Thread(
        target=serial_reader,
        args=(port, baud, data_queue, stop_event),
        daemon=True,
    )
    reader_thread.start()

    # Circular buffers (keep only last <window> seconds of data)
    max_points = int(window / 0.05) + 1000  # generous upper bound
    times: deque[float] = deque(maxlen=max_points)
    rpms: deque[int] = deque(maxlen=max_points)

    fig, ax = plt.subplots(figsize=(14, 5))
    (line,) = ax.plot([], [], linewidth=1.2, color="royalblue", label="Motor RPM")

    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Motor RPM")
    ax.set_title(f"Live Motor RPM — {port}")
    ax.legend(loc="upper left")
    ax.grid(True, linestyle="--", alpha=0.5)

    start_time: list[float | None] = [None]  # mutable container for closure

    def update(_frame: int) -> tuple:
        # Drain all queued samples
        changed = False
        while not data_queue.empty():
            try:
                ts, rpm = data_queue.get_nowait()
            except queue.Empty:
                break
            if start_time[0] is None:
                start_time[0] = ts
            times.append(ts - start_time[0])
            rpms.append(rpm)
            changed = True

        if not changed or not times:
            return (line,)

        t_arr = list(times)
        r_arr = list(rpms)

        line.set_data(t_arr, r_arr)

        # Scroll: show last <window> seconds
        t_max = t_arr[-1]
        t_min = max(0.0, t_max - window)
        ax.set_xlim(t_min, t_max + 1.0)

        r_visible = [r for t, r in zip(t_arr, r_arr) if t >= t_min]
        y_max = max(r_visible) if r_visible else 100
        ax.set_ylim(0, y_max * 1.15 + 100)

        return (line,)

    ani = animation.FuncAnimation(
        fig,
        update,
        interval=100,   # redraw every 100 ms
        blit=False,
        cache_frame_data=False,
    )

    try:
        plt.tight_layout()
        plt.show()
    finally:
        stop_event.set()
        reader_thread.join(timeout=3)

    # Keep ani alive (prevents garbage collection before show() returns)
    _ = ani


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="Live Motor RPM plot from serial device.")
    parser.add_argument("--port",   default="/dev/ttyACM0", help="Serial port (default: /dev/ttyACM0)")
    parser.add_argument("--baud",   type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--window", type=float, default=60.0, help="Visible time window in seconds (default: 60)")
    args = parser.parse_args()

    run_live_plot(args.port, args.baud, args.window)


if __name__ == "__main__":
    main()
