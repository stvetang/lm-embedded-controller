import serial
import re
import csv
from datetime import datetime

PORT = "COM3"
BAUD = 115200
SAMPLE_COUNT = 50
MAX_STEP_ALLOWED = 30        # Max step allowed between actuator values
CONVERGENCE_THRESHOLD = 5    # Max difference between sensor and actuator values
CONVERGENCE_WINDOW = 5       # Number of consecutive samples checked for convergence

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
        

def run_checks(samples, lable):
    stability_pass = all(
        abs(samples[i][1] - samples[i - 1][1]) <= MAX_STEP_ALLOWED
        for i in range(1, len(samples))
    )
    range_pass = all(0 <= actuator <= 100 for _, actuator in samples)
    recent = samples[-CONVERGENCE_WINDOW:]
    convergence_pass = all(
        abs(sensor - actuator) <= CONVERGENCE_THRESHOLD for sensor, actuator in recent
    )

    print(f"\n=== {lable} ===")
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