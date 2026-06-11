#!/usr/bin/env python3
"""
Lu.i Neuron Calibration -- Python host script.

Sends a frequency-sweep command to the Arduino, reads the "f_in,f_out" pairs it
streams back, plots input frequency vs output frequency, and fits a line. The
slope of that line is the synaptic weight (R_w = f_out / f_in, the Banerjee
calibration). A near-zero intercept confirms a clean weighted synapse.

Edit the CONFIG section below, then run:
    python lui_calibration.py

Requires: pyserial, numpy, matplotlib
    pip install pyserial numpy matplotlib
"""

import sys
import time

import numpy as np
import serial
import matplotlib.pyplot as plt

# ============================================================================
# CONFIG -- edit these values
# ============================================================================
PORT       = "COM3"     # serial port
BAUD       = 115200     # must match the Arduino sketch
FREQ_MIN   = 5.0        # minimum input frequency (Hz)
FREQ_MAX   = 15.0       # maximum input frequency (Hz)
FREQ_STEP  = 1        # frequency step (Hz)
SETTLE_MS  = 2000       # settle time per step (ms) before counting
MEASURE_MS = 4000       # measurement window per step (ms)
SAVE_PATH  = None       # e.g. "calibration.png", or None to skip saving
# ============================================================================


def run_calibration():
    print(f"Opening {PORT} at {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=2)
    time.sleep(2.0)  # wait for the Arduino auto-reset on connect
    ser.reset_input_buffer()

    # Send sweep parameters
    cmd = f"{FREQ_MIN},{FREQ_MAX},{FREQ_STEP},{SETTLE_MS},{MEASURE_MS}\n"
    ser.write(cmd.encode())
    print(f"Sent sweep command: {cmd.strip()}")
    print("Collecting data (this takes a while)...\n")

    input_freqs = []
    output_freqs = []

    while True:
        raw = ser.readline().decode(errors="ignore").strip()
        if not raw:
            continue
        if raw == "DONE":
            break
        try:
            f_in, f_out = (float(v) for v in raw.split(","))
        except ValueError:
            print(f"  (ignored line: {raw!r})")
            continue
        input_freqs.append(f_in)
        output_freqs.append(f_out)
        print(f"  input {f_in:6.2f} Hz  ->  output {f_out:6.2f} Hz")

    ser.close()
    return np.array(input_freqs), np.array(output_freqs)


def fit_and_plot(input_freqs, output_freqs):
    if len(input_freqs) < 2:
        print("Not enough data points to fit a line.")
        return

    # Linear fit: f_out = slope * f_in + intercept
    slope, intercept = np.polyfit(input_freqs, output_freqs, 1)

    # R^2 for fit quality
    predicted = slope * input_freqs + intercept
    ss_res = np.sum((output_freqs - predicted) ** 2)
    ss_tot = np.sum((output_freqs - np.mean(output_freqs)) ** 2)
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else float("nan")

    print("\n" + "=" * 40)
    print(f"  Weight (slope):  {slope:.4f}")
    print(f"  Intercept:       {intercept:.3f} Hz")
    print(f"  R^2:             {r2:.4f}")
    print("=" * 40)
    if abs(intercept) > 2.0:
        print("  Note: nonzero intercept suggests a bias/offset or a")
        print("        nonlinear operating point. Calibrate near the")
        print("        frequency range you will actually use.")

    plt.figure(figsize=(8, 6))
    plt.scatter(input_freqs, output_freqs, color="#1f77b4", s=50, zorder=3,
                label="Measured")
    x_line = np.linspace(input_freqs.min(), input_freqs.max(), 100)
    plt.plot(x_line, slope * x_line + intercept, "r--", zorder=2,
             label=f"Fit: weight = {slope:.3f}  (R\u00b2 = {r2:.3f})")
    plt.xlabel("Input frequency (Hz)")
    plt.ylabel("Output frequency (Hz)")
    plt.title("Lu.i Neuron Calibration: Input vs Output Frequency")
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    if SAVE_PATH:
        plt.savefig(SAVE_PATH, dpi=150)
        print(f"\nPlot saved to {SAVE_PATH}")
    plt.show()


def main():
    try:
        in_f, out_f = run_calibration()
    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        sys.exit(1)
    fit_and_plot(in_f, out_f)
    
    sys.exit(1)

if __name__ == "__main__":
    main()