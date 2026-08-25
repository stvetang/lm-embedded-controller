import serial
import re
import csv
from datetime import datetime

PORT = "COM3"
BAUD = 115200
SAMPLE_COUNT = 50
MAX_STEP_ALLOWED = 30           # Stability: Max change allowed between actuator values
CONVERGENCE_THRESHOLD = 8       # Max difference between sensor and actuator values. How well the actuator "tracks" the sensor
CONVERGENCE_WINDOW = 9          # Number of samples checked for convergence
CONVERGENCE_MIN_FRACTION = .75  # How much of the convergence window must be close to pass  


pattern = re.compile(r"sensor: (\d+) actuator: (\d+)")


def send_command(ser, command):
    ser.write((command + "\n").encode("utf-8"))
    print(f"Sent command: {command}")


def collect_samples(ser, count, log_writer=None):
    samples = []
    while len(samples) < count:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        match = pattern.match(line)
        if match:
            sensor_val = int(match.group(1))
            actuator_val = int(match.group(2))
            timestamp = datetime.now().isoformat()
            print(f"{timestamp} | sensor={sensor_val} actuator={actuator_val}")
            if log_writer:
                log_writer.writerow([timestamp, sensor_val, actuator_val])
            samples.append((sensor_val, actuator_val))
    return samples
        

def check_convergence(samples):
    for start in range(len(samples) - CONVERGENCE_WINDOW + 1):
        window = samples[start:start + CONVERGENCE_WINDOW]
        close_count = sum(
            1 for sensor, actuator in window if abs(sensor - actuator) <= CONVERGENCE_THRESHOLD
        )
        if (close_count / len(window)) >= CONVERGENCE_MIN_FRACTION:
            return True
        return False


def run_checks(samples, label):
    # ---- Stability check ----
    # Does the actuator's movement look like smooth, controlled motion.
    stability_pass = all(
        abs(samples[i][1] - samples[i - 1][1]) <= MAX_STEP_ALLOWED
        for i in range(1, len(samples))
    )

    # ---- Range check ----  
    # Actuator readings should only fall within 0-100.  
    range_pass = all(0 <= actuator <= 100 for _, actuator in samples)
    recent = samples[-CONVERGENCE_WINDOW:]
    close_count = sum(
        1 for sensor, actuator in recent if abs(sensor - actuator) <= CONVERGENCE_THRESHOLD
    )

    # ---- Convergence check ----
    # Is the controller able to sometimes track its target within reason.
    convergence_pass = check_convergence(samples)

    print(f"\n=== {label} ===")
    print(f"Stability check: {'PASS' if stability_pass else 'FAIL'}")
    print(f"Range check: {'PASS' if range_pass else 'FAIL'}")
    print(f"Convergence check: {'PASS' if convergence_pass else 'FAIL'}")

    passed = sum([stability_pass, range_pass, convergence_pass])
    print(f"{passed}/3 tests passed.")
    return passed == 3


# ---- Fault injection test ----
def run_fault_injection_test(ser):
    print("\nRunning fault injection: out-of-range sensor value (10000)")
    send_command(ser, "SET 10000")
    samples = collect_samples(ser, SAMPLE_COUNT)
    result = run_checks(samples, "FAULT INJECTION TEST: sensor = 10000")
    send_command(ser, "CLEAR")
    return result

if __name__ == "__main__":
    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"Listening on {PORT} at {BAUD} baud...")

    with open ("log.csv", "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["timestamp", "sensor", "actuator"])

        print("\n=== NORMAL OPERATION TEST ===")
        normal_samples = collect_samples(ser, SAMPLE_COUNT, log_writer=writer)
        run_checks(normal_samples, "NOMRAL OPERATION")

    run_fault_injection_test(ser)