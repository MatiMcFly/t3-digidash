# Motor RPM Tools

Two Python scripts for capturing and visualising motor RPM data from a serial device.

## Requirements

```bash
pip install -r requirements.txt
```

---

## `live_motor_rpm.py` — Live serial monitor

Connects to a serial port, parses incoming datasets and displays a continuously
scrolling RPM plot in real time.

```
usage: live_motor_rpm.py [--port PORT] [--baud BAUD] [--window SECONDS]

options:
  --port    Serial device to read from   (default: /dev/ttyACM0)
  --baud    Baud rate                    (default: 115200)
  --window  Visible time window (s)      (default: 60)
```

**Examples**

```bash
# Use defaults (/dev/ttyACM0, 115200 baud, 60 s window)
python3 live_motor_rpm.py

# USB adapter at 9600 baud, show last 2 minutes
python3 live_motor_rpm.py --port /dev/ttyUSB0 --baud 9600 --window 120
```

---

## `plot_motor_rpm.py` — Log file plotter

Parses a minicom log file and plots the recorded RPM values over time.
Prints basic statistics (min, max, mean RPM) to the terminal.

```
usage: plot_motor_rpm.py [logfile] [--interval SECONDS]

arguments:
  logfile     Path to the minicom log file  (default: data/minicom-output.log)

options:
  --interval  Sample interval in seconds    (default: 0.1)
```

**Examples**

```bash
# Use the default log file (data/minicom-output.log)
python3 plot_motor_rpm.py

# Specify a different log file
python3 plot_motor_rpm.py data/my_run.log

# Log recorded at 20 Hz (0.05 s per sample)
python3 plot_motor_rpm.py data/my_run.log --interval 0.05
```

---

## Expected serial / log format

Both tools expect datasets delimited by a `Turn Signal:` line, with an optional
`Motor RPM:` line inside each dataset:

```
Turn Signal: ...
Motor RPM:   1450 rpm
...
Turn Signal: ...
Motor RPM:   1480 rpm
...
```

Datasets missing a `Motor RPM:` line are recorded as **0 RPM**.
