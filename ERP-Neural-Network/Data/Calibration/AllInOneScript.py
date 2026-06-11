# -*- coding: utf-8 -*-
"""
Created on Wed May 27 13:52:07 2026

@author: Vincent
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import differential_evolution, minimize
from scipy.ndimage import gaussian_filter1d

# ─── Fixed parameters ─────────────────────────────────────────────────────────
V_LEAK      = 0.650
V_RESET     = 0.0
REFRAC_TIME = 0.012

# ─── LIF simulation ───────────────────────────────────────────────────────────
# Module-level so it can be pickled by multiprocessing on Windows.
# Handles any number of threshold crossings automatically.
def SimulateLIF(time, tau_mem, weight, tau_syn, t_pre, V_thresh):
    dt_arr = np.diff(time)
    V = np.empty(len(time))
    V[0] = V_LEAK
    in_refrac  = False
    refrac_end = -np.inf
    for i in range(1, len(time)):
        t_curr = time[i]
        if in_refrac:
            V[i] = V_RESET
            if t_curr >= refrac_end:
                in_refrac = False
            continue
        I_syn = weight * np.exp(-(t_curr - t_pre) / tau_syn) if t_curr > t_pre else 0.0
        V[i]  = V[i - 1] + (-(V[i - 1] - V_LEAK) + I_syn) / tau_mem * dt_arr[i - 1]
        if V[i] >= V_thresh:
            V[i]       = V_RESET
            in_refrac  = True
            refrac_end = t_curr + REFRAC_TIME
    return V

# ─── Picklable objective class ────────────────────────────────────────────────
# A class with __call__ is picklable even on Windows, unlike a closure.
# This allows workers=-1 (all CPU cores) in differential_evolution.
class Objective:
    def __init__(self, time, V_mem, fit_mask, V_thresh):
        self.time     = time
        self.V_mem    = V_mem
        self.fit_mask = fit_mask
        self.V_thresh = V_thresh

    def __call__(self, params):
        tau_mem, weight, tau_syn, t_pre = params
        if tau_syn <= 0 or weight <= 0:
            return 1e10
        V_sim = SimulateLIF(self.time, tau_mem, weight, tau_syn, t_pre, self.V_thresh)
        return np.sum((V_sim[self.fit_mask] - self.V_mem[self.fit_mask]) ** 2)

# ─── Detect ALL spike events ──────────────────────────────────────────────────
def DetectAllSpikes(time, V_mem):
    smoothed = gaussian_filter1d(V_mem, sigma=5)
    dV       = np.diff(smoothed)
    dt       = np.diff(time).mean()

    # Only look for drops where V_mem is actually elevated above V_leak
    elevated_mask = smoothed > V_LEAK + 0.05
    dV_work = dV.copy()
    dV_work[~elevated_mask[:-1]] = 0.0

    min_sep    = int(0.020 / dt)  # spikes must be at least 20 ms apart
    drop_threshold = -0.5 * np.abs(dV_work.min())  # adaptive: 50% of the largest drop

    spike_drop_indices = []
    while True:
        idx = np.argmin(dV_work)
        if dV_work[idx] >= drop_threshold or dV_work[idx] >= 0:
            break
        spike_drop_indices.append(idx)
        lo = max(0, idx - min_sep)
        hi = min(len(dV_work), idx + min_sep)
        dV_work[lo:hi] = 0.0

    spike_drop_indices.sort()
    return spike_drop_indices

def DetectEvents(time, V_mem):
    dt              = np.diff(time).mean()
    drop_indices    = DetectAllSpikes(time, V_mem)

    if not drop_indices:
        raise ValueError("No spike detected — check data or lower elevated_threshold")

    # V_thresh: peak of raw data in 100 ms before the first spike
    first_drop = drop_indices[0]
    lookback   = min(first_drop, int(0.1 / dt))
    V_thresh   = V_mem[first_drop - lookback : first_drop + 1].max()

    # For each spike find the reset minimum in 25 ms after the drop
    reset_indices = []
    for drop_idx in drop_indices:
        post = slice(drop_idx + 1, min(len(time), drop_idx + int(0.025 / dt)))
        reset_indices.append(drop_idx + 1 + np.argmin(V_mem[post]))

    return V_thresh, drop_indices, reset_indices

# ─── Fit mask: exclude the transition for every spike ─────────────────────────
def MakeFitMask(time, drop_indices, reset_indices):
    dt   = np.diff(time).mean()
    mask = np.ones(len(time), dtype=bool)
    for drop_idx, reset_idx in zip(drop_indices, reset_indices):
        lo = max(0, drop_idx - 2)
        hi = min(len(time), reset_idx + int(0.005 / dt))
        mask[lo:hi] = False
    return mask

# ─── Main ─────────────────────────────────────────────────────────────────────
def DoFit():
    data  = np.genfromtxt('Meting2.csv', delimiter=',')
    time  = data[:, 0]
    V_mem = data[:, 1]

    mask  = time > time[0] + 0.001
    time  = time[mask]
    V_mem = V_mem[mask]

    V_thresh, drop_indices, reset_indices = DetectEvents(time, V_mem)
    fit_mask = MakeFitMask(time, drop_indices, reset_indices)

    n_spikes = len(drop_indices)
    print(f"Detected  V_thresh = {V_thresh:.4f} V")
    print(f"          spikes   = {n_spikes}  at t = {[f'{time[d]*1000:.1f}ms' for d in drop_indices]}")
    print(f"Fitting {fit_mask.sum()} / {len(time)} samples\n")

    objective = Objective(time, V_mem, fit_mask, V_thresh)

    t_bound_lo = time[0] - 0.05
    t_bound_hi = time[drop_indices[0]]
    bounds = [(0.01, 0.5), (0.01, 42.0), (0.04, 0.5), (t_bound_lo, t_bound_hi)]

    print("Running global search (differential_evolution, all cores)...")
    global_result = differential_evolution(
        objective, bounds, seed=42, maxiter=1000, tol=1e-9, polish=False, workers=-1
    )
    print(f"Global:  score={global_result.fun:.6f}  params={global_result.x}")

    print("Running local refinement (Nelder-Mead)...")
    local_result = minimize(
        objective, global_result.x, method='Nelder-Mead',
        options={'maxiter': 20000, 'xatol': 1e-8, 'fatol': 1e-12}
    )

    tau_mem_fit, weight_fit, tau_syn_fit, t_pre_fit = local_result.x

    print(f"\nFit results:")
    print(f"  weight   = {weight_fit:.4f} V")
    print(f"  tau_syn  = {tau_syn_fit * 1000:.2f} ms")
    print(f"  t_pre    = {t_pre_fit * 1000:.2f} ms")
    print(f"  tau_mem  = {tau_mem_fit * 1000:.2f} ms  (fixed)")
    print(f"  Score    = {local_result.fun:.8f}")

    V_sim = SimulateLIF(time, tau_mem_fit, weight_fit, tau_syn_fit, t_pre_fit, V_thresh)
    residuals = (V_sim - V_mem).copy()
    residuals[~fit_mask] = np.nan

    fig, axes = plt.subplots(1, 1, figsize=(11, 8), sharex=True)
    dt = np.diff(time).mean()

    ax = axes
    ax.plot(time * 1000, V_mem, label='Data',       alpha=0.7, linewidth=0.8, color='steelblue')
    ax.plot(time * 1000, V_sim, label='Simulation', linewidth=1.8,            color='darkorange')
    ax.axhline(V_thresh, color='red',  linestyle='--', linewidth=0.8, label=f'$V_{{thresh}}$ = {V_thresh:.3f} V')
    ax.axhline(V_LEAK,   color='gray', linestyle='--', linewidth=0.8, label=f'$V_{{leak}}$ = {V_LEAK:.3f} V')
    for drop_idx, reset_idx in zip(drop_indices, reset_indices):
        lo = time[max(0, drop_idx - 2)] * 1000
        hi = time[min(reset_idx + int(0.005 / dt), len(time) - 1)] * 1000
        ax.axvspan(lo, hi, alpha=0.12, color='red')
    ax.axvspan(0, 0, alpha=0.12, color='red', label='Excluded from fit')
    ax.set_ylabel('$V_{mem}$ (V)')
    ax.set_title(
        f'{n_spikes} spike(s) — weight = {weight_fit:.3f} V   '
        f'$\\tau_{{syn}}$ = {tau_syn_fit * 1000:.1f} ms   '
        f'$\\tau_{{mem}}$ = {tau_mem_fit * 1000:.1f} ms'
    )
    ax.legend(fontsize=8, ncol=3)

    '''
    ax = axes[1]
    ax.plot(time * 1000, residuals * 1000, linewidth=0.8, color='steelblue')
    ax.axhline(0, color='k', linewidth=0.5)
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('Residual (mV)')
    ax.set_title('Residuals (spike transitions excluded)')
    '''
    plt.tight_layout()
    plt.savefig('Fit.png', dpi=400)
    plt.show()

# Windows multiprocessing requires the __main__ guard —
# without it every worker process would re-run main() causing a crash.
if __name__ == '__main__':
    DoFit()