# LM Embedded Controller(In Progress)

A real-time embedded control system built on an STM32 microcontroller, paired with a Python-based test harness that mirrors Hardware-In-The-Loop (HIL) style validation used in embedded/flight software testing.

## Overview

This project simulates a closed-loop control system: an STM32 continuously reads a simulated sensor value, runs it through a proportional controller, and drives a simulated actuator toward that target in real time. A companion Python harness listens over serial, logs the data, and runs automated pass/fail checks against the system's behavior.

The goal was to build something closer to real embedded systems engineering than a typical hobby project — real-time constraints, HAL-level hardware control, and an actual automated test suite rather than manual observation.

## Architecture

- **Firmware (C, STM32 HAL):** generates a simulated sensor reading each cycle, runs a proportional control loop to move a virtual actuator toward it, and streams both values over UART.
- **Test Harness (Python):** reads the serial stream, logs data to CSV, and runs automated checks:
  - **Range check** — actuator stays within expected physical bounds
  - **Stability check** — no erratic/runaway jumps between readings
  - **Convergence check** — actuator sustains close tracking of the sensor value, not just a lucky single match

## Tech Stack

- **Firmware:** C, STM32 HAL, PlatformIO
- **Hardware:** STM32 Nucleo-32 (F303K8)
- **Test Harness:** Python, pyserial

## Example Output

sensor: 89 actuator: 89
sensor: 79 actuator: 87
sensor: 74 actuator: 84
...
Stability check: PASS
Range check: PASS
Convergence check: PASS
3/3 tests passed.

## Setup

1. Open `lm-embedded-controller/` in VS Code with the PlatformIO extension installed.
2. Build and upload to an STM32 Nucleo-32 F303K8 board.
3. Install harness dependencies: `pip install pyserial`
4. Update the `PORT` variable in `harness.py` to match your board's COM port.
5. Run `python harness.py`.

