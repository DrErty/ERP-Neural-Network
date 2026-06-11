# -*- coding: utf-8 -*-
"""
Created on Wed May 27 13:52:07 2026

@author: Vincent
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import differential_evolution, minimize
from scipy.ndimage import gaussian_filter1d
from numba import njit
import time as time_lib

# ─── Fixed parameters ─────────────────────────────────────────────────────────
#TAU_MEM     = 0.14743556
V_LEAK      = 0.645
V_RESET     = 0.0
REFRAC_TIME = 0.012

# ─── LIF simulation ───────────────────────────────────────────────────────────
# Module-level so it can be pickled by multiprocessing on Windows.
# Handles any number of threshold crossings automatically.
@njit
def SimulateLIF(time, weight, tau_syn, t_pre, V_thresh, tau_mem):
    dt_arr     = np.diff(time)
    exp_mem    = np.exp(-dt_arr / tau_mem)
    exp_syn    = np.exp(-dt_arr / tau_syn)
    syn_factor = tau_syn / (tau_syn - tau_mem)

    V          = np.empty(len(time))
    V[0]       = V_LEAK
    in_refrac  = False
    refrac_end = -np.inf

    for i in range(1, len(time)):
        t_curr = time[i]
        if in_refrac:
            V[i] = V_RESET
            if t_curr >= refrac_end:
                in_refrac = False
            continue

        I_syn = weight * np.exp(-(time[i - 1] - t_pre) / tau_syn) if time[i - 1] > t_pre else 0.0
        A     = exp_mem[i - 1]
        B     = exp_syn[i - 1]
        V[i]  = V_LEAK + (V[i - 1] - V_LEAK) * A + I_syn * syn_factor * (B - A)

        if V[i] >= V_thresh:
            V[i]       = V_RESET
            in_refrac  = True
            refrac_end = t_curr + REFRAC_TIME

    return V

# ─── Picklable objective class ────────────────────────────────────────────────
# A class with __call__ is picklable even on Windows, unlike a closure.
# This allows workers=-1 (all CPU cores) in differential_evolution.
class Objective:
    def __init__(self, time, V_mem, fit_mask, V_thresh_estimate):
        self.time             = time
        self.V_mem            = V_mem
        self.fit_mask         = fit_mask
        self.V_thresh_estimate = V_thresh_estimate

    def __call__(self, params):
        weight, tau_syn, t_pre, V_thresh, tau_mem = params
        if tau_syn <= 0 or weight <= 0 or V_thresh <= V_LEAK:
            return 1e10
        V_sim = SimulateLIF(self.time, weight, tau_syn, t_pre, V_thresh, tau_mem)
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
    drop_threshold = -0.5 * np.abs(dV_work.max())  # adaptive: 50% of the largest drop

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
        return 0.9, [], []
        #raise ValueError("No spike detected — check data or lower elevated_threshold")

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
    data  = np.genfromtxt('Meting3.csv', delimiter=',')
    time  = data[:, 0]
    V_mem = data[:, 1]
    
    time_mask = np.logical_and(time > -0.500, time < 1)
    mask  = np.logical_and(time > time[0] + 0.001, time_mask)
    time  = time[mask]
    V_mem = V_mem[mask]

    V_thresh, drop_indices, reset_indices = DetectEvents(time, V_mem)
    fit_mask = MakeFitMask(time, drop_indices, reset_indices)
    
    # TEMP FIX TODO
    V_thresh = V_mem.max()

    n_spikes = len(drop_indices)
    print(f"Detected  V_thresh = {V_thresh:.4f} V")
    print(f"          spikes   = {n_spikes}  at t = {[f'{time[d]*1000:.1f}ms' for d in drop_indices]}")
    print(f"Fitting {fit_mask.sum()} / {len(time)} samples\n")

    t_bound_lo = time[0] - 0.05
    t_bound_hi = time[drop_indices[0] if len(drop_indices) > 0 else len(time) - 1]
    bounds = [(0.01, 8), (0.150, 0.2), (t_bound_lo, t_bound_hi)]
    
    objective = Objective(time, V_mem, fit_mask, V_thresh)
    
    #TAU_MEM_MEASURED = 0.14743556
    TAU_MEM_MEASURED = 0.1273
    
    bounds = [
        (0.01, 3.0),                                            # weight
        (0.01, 3.0),                                            # tau_syn
        (t_bound_lo, t_bound_hi),                               # t_pre
        (V_thresh - 0.05, V_thresh + 0.05),                     # V_thresh
        (TAU_MEM_MEASURED - 0.0002, TAU_MEM_MEASURED + 0.0002),     # tau_mem
    ]
    
    start_time = time_lib.time()
    
    run_fit = True;
    if run_fit:
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
    
        weight_fit, tau_syn_fit, t_pre_fit, V_thresh_fit, tau_mem_fit = local_result.x
    else:
        weight_fit, tau_syn_fit, t_pre_fit, V_thresh_fit, tau_mem_fit = 1.6, 0.11562, 8 / 1000, 0.7648, 149.16 / 1000
    
    end_time = time_lib.time()
    
    elapsed_time = end_time - start_time
    print("Elapsed time: ", elapsed_time) 
    
    print(f"\nFit results:")
    print(f"  weight   = {weight_fit:.4f} V")
    print(f"  tau_syn  = {tau_syn_fit * 1000:.2f} ms")
    print(f"  t_pre    = {t_pre_fit * 1000:.2f} ms")
    #print(f"  tau_mem  = {TAU_MEM * 1000:.2f} ms  (fixed)")
    print(f"  V_thresh_fit  = {V_thresh_fit:.4f} V")
    print(f"  tau_mem_fit  = {tau_mem_fit * 1000:.2f} ms")
    if run_fit:
        print(f"  Score    = {local_result.fun:.8f}")

    V_sim = SimulateLIF(time, weight_fit, tau_syn_fit, t_pre_fit, V_thresh, tau_mem_fit)
    residuals = (V_sim - V_mem).copy()
    residuals[~fit_mask] = np.nan

    #fig, axes = plt.subplots(2, 1, figsize=(11, 8), sharex=True)
    fig, axes = plt.subplots(1, 1, figsize=(11, 8), sharex=True)
    dt = np.diff(time).mean()

    ax = axes
    #ax = axes[0]
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
        f'$V_{{thresh}}$ = {V_thresh_fit * 1000:.1f} mV'
        f'$\\tau_{{mem}}$ = {tau_mem_fit * 1000:.1f} ms   '
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