#!/usr/bin/env python3
"""
Plot Motor RPM from minicom log file.

The log contains repeated datasets arriving every 0.1 s.
Each dataset starts with a 'Turn Signal:' line and optionally contains
a 'Motor RPM:' line. Missing Motor RPM values are treated as 0.
"""

import re
import argparse
import matplotlib.pyplot as plt


def parse_log(filepath: str) -> list[int]:
    """
    Parse the log file and return a list of Motor RPM values,
    one per dataset (0.1 s interval).
    """
    rpm_pattern = re.compile(r"Motor RPM:\s+(\d+)\s+rpm")

    rpm_values: list[int] = []
    current_rpm: int | None = None

    with open(filepath, "r", errors="replace") as fh:
        for line in fh:
            line = line.strip()

            # A 'Turn Signal:' line marks the start of a new dataset.
            # Flush the previous dataset when we see one (except the very first).
            if line.startswith("Turn Signal:"):
                if current_rpm is not None:
                    # Save the completed dataset's RPM
                    rpm_values.append(current_rpm)
                # Start a new dataset with default RPM = 0
                current_rpm = 0
                continue

            if current_rpm is not None:
                m = rpm_pattern.match(line)
                if m:
                    current_rpm = int(m.group(1))

    # Flush the last dataset
    if current_rpm is not None:
        rpm_values.append(current_rpm)

    return rpm_values


def plot_rpm(rpm_values: list[int], sample_interval: float = 0.1) -> None:
    """Plot Motor RPM over time."""
    n = len(rpm_values)
    time_axis = [i * sample_interval for i in range(n)]

    fig, ax = plt.subplots(figsize=(14, 5))
    ax.plot(time_axis, rpm_values, linewidth=1.0, color="royalblue", label="Motor RPM")

    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Motor RPM")
    ax.set_title("Motor RPM over Time")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.5)

    # Print basic statistics
    valid = [v for v in rpm_values if v > 0]
    print(f"Total datasets  : {n}")
    print(f"Duration        : {(n - 1) * sample_interval:.1f} s")
    print(f"Datasets w/ RPM : {len(valid)}")
    if valid:
        print(f"RPM  min        : {min(valid)}")
        print(f"RPM  max        : {max(valid)}")
        print(f"RPM  mean       : {sum(valid) / len(valid):.1f}")

    plt.tight_layout()
    plt.show()


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot Motor RPM from minicom log.")
    parser.add_argument(
        "logfile",
        nargs="?",
        default="data/minicom-output.log",
        help="Path to the minicom log file (default: data/minicom-output.log)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.1,
        help="Sample interval in seconds (default: 0.1)",
    )
    args = parser.parse_args()

    rpm_values = parse_log(args.logfile)
    plot_rpm(rpm_values, sample_interval=args.interval)


if __name__ == "__main__":
    main()
