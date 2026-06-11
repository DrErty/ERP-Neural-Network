import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import differential_evolution, minimize
from scipy.ndimage import gaussian_filter1d
#from numba import njit
import time as time_lib

DATA_FILES = [
    'MetingJune1.csv',
    #'Meting2.csv',
    # 'Meting5.csv',
]

TIME_WINDOW = (-0.1, 0.6)
DOWNSAMPLE = 20

V_LEAK = 0.738
V_RESET = 0
REFRAC_TIME = 0.012

ADD_TAU_MEM_PARAM = False
MEASURED_TAU_MEM = 0.02476464

FIXED_TAU_SYN = False
MEASURED_TAU_SYN = 0.08151

#@njit
def SimulateLIF(time, weight, tau_syn, t_pre, V_thresh, tau_mem):
    if FIXED_TAU_SYN:
        tau_syn = MEASURED_TAU_SYN
    
    dt_arr = np.diff(time)
    exp_mem = np.exp(-dt_arr / tau_mem)
    exp_syn = np.exp(-dt_arr / tau_syn)
    syn_factor = tau_syn / (tau_syn - tau_mem)

    V = np.empty(len(time))
    V[0] = V_LEAK
    in_refrac = False
    refrac_end = -np.inf

    drop_indices_sim = []

    for i in range(1, len(time)):
        t_curr = time[i]
        if in_refrac:
            V[i] = V_RESET
            if t_curr >= refrac_end:
                in_refrac = False
            continue
        
        I_syn = weight * np.exp(-(time[i - 1] - t_pre) / tau_syn) if time[i - 1] > t_pre else 0.0
        A = exp_mem[i - 1]
        B = exp_syn[i - 1]
        V[i]  = V_LEAK + (V[i - 1] - V_LEAK) * A + I_syn * syn_factor * (B - A)
        
        if V[i] >= V_thresh:
            V[i]       = V_RESET
            in_refrac  = True
            drop_indices_sim.append(i)
            refrac_end = t_curr + REFRAC_TIME

    return V, drop_indices_sim

class MultiObjective:
    def __init__(self, times, V_mems, fit_masks, V_thresh_estimate, t_pres, drop_indices):
        self.times = times
        self.V_mems = V_mems
        self.fit_masks = fit_masks
        self.V_thresh_estimate = V_thresh_estimate
        self.t_pres = t_pres
        self.drop_indices = drop_indices
        
    def __call__(self, params):
        tau_syn  = params[0]
        tau_mem  = params[1]
        V_thresh = self.V_thresh_estimate
        
        if not ADD_TAU_MEM_PARAM:
            tau_mem = MEASURED_TAU_MEM

        if tau_syn <= 0 or tau_mem <= 0 or V_thresh <= V_LEAK:
            return 1e10
        if abs(tau_syn - tau_mem) < 1e-6:
            return 1e10

        total = 0.0
        for i in range(len(self.times)):
            weight = params[2 + i]
            if weight <= 0:
                return 1e10
            V_sim, drop_indices_sim = SimulateLIF(self.times[i], weight, tau_syn, self.t_pres[i], V_thresh, tau_mem)
            diff = V_sim[self.fit_masks[i]] - self.V_mems[i][self.fit_masks[i]]
            total += float(np.dot(diff, diff))
            
            if (len(drop_indices_sim) != len(self.drop_indices[i])):
                return 1e10
            
            for drop, drop_sim in zip(self.drop_indices[i], drop_indices_sim):
                total += np.abs(drop - drop_sim) * 0.01
            
        return total

# ─── Event detection ──────────────────────────────────────────────────────────
def DetectAllSpikes(time, V_mem):
    smoothed = gaussian_filter1d(V_mem, sigma=5)
    dV       = np.diff(smoothed)
    dt       = np.diff(time).mean()

    elevated_mask = smoothed > V_LEAK + 0.05
    dV_work = dV.copy()
    dV_work[~elevated_mask[:-1]] = 0.0

    min_sep = int(0.020 / dt)
    min_val = dV_work.min()

    # A real spike reset drops V_mem by ~0.8 V; even after Gaussian smoothing
    # the per-sample drop is well above 0.05 V. If the largest drop in the
    # elevated region is smaller than this, there are no real spikes — only
    # noise — and the relative threshold would otherwise trigger false detections.
    if abs(min_val) < 0.01:
        return []

    drop_threshold = -0.5 * abs(min_val)

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
    dt           = np.diff(time).mean()
    drop_indices = DetectAllSpikes(time, V_mem)

    if not drop_indices:
        return float(V_mem.max()), [], []

    first_drop = drop_indices[0]
    smoothed   = gaussian_filter1d(V_mem, sigma=5)
    lookback   = min(first_drop, int(0.005 / dt))
    V_thresh   = float(smoothed[first_drop - lookback : first_drop + 1].max())

    reset_indices = []
    for drop_idx in drop_indices:
        post = slice(drop_idx + 1, min(len(time), drop_idx + int(0.025 / dt)))
        reset_indices.append(drop_idx + 1 + int(np.argmin(V_mem[post])))

    return V_thresh, drop_indices, reset_indices

def MakeFitMask(time, drop_indices, reset_indices):
    dt   = np.diff(time).mean()
    mask = np.ones(len(time), dtype=bool)
    for drop_idx, reset_idx in zip(drop_indices, reset_indices):
        lo = max(0, drop_idx - 2)
        hi = min(len(time), reset_idx + int(0.005 / dt))
        mask[lo:hi] = False
    return mask

# ─── Load and preprocess one file ─────────────────────────────────────────────
def LoadFile(filename):
    data  = np.genfromtxt(filename, delimiter=',')
    time  = data[:, 0]
    V_mem = data[:, 1]

    keep  = (time > time[0] + 0.001) & (time > TIME_WINDOW[0]) & (time < TIME_WINDOW[1])
    time  = time[keep]
    V_mem = V_mem[keep]

    V_thresh, drop_indices, reset_indices = DetectEvents(time, V_mem)

    # Downsample for fitting, keep full resolution for plotting
    time_ds  = time[::DOWNSAMPLE].copy()
    V_mem_ds = V_mem[::DOWNSAMPLE].copy()

    # Map spike times to nearest indices in downsampled array
    spike_times = [time[d] for d in drop_indices]
    reset_times = [time[r] for r in reset_indices]
    drop_ds  = [int(np.argmin(np.abs(time_ds - t))) for t in spike_times]
    reset_ds = [int(np.argmin(np.abs(time_ds - t))) for t in reset_times]

    fit_mask   = MakeFitMask(time_ds, drop_ds, reset_ds)
    t_bound_lo = float(time_ds[0]) - 0.05
    t_bound_hi = float(time_ds[drop_ds[0]]) if drop_ds else float(time_ds[-1])

    print(f"  {filename}: {len(drop_indices)} spike(s), "
          f"V_thresh ≈ {V_thresh:.4f} V, "
          f"fitting {fit_mask.sum()}/{len(time_ds)} downsampled samples")

    return {
        'filename'     : filename,
        'time_full'    : time,
        'V_mem_full'   : V_mem,
        'time_ds'      : time_ds,
        'V_mem_ds'     : V_mem_ds,
        'fit_mask'     : fit_mask,
        'V_thresh'     : V_thresh,
        'drop_indices' : drop_indices,
        'reset_indices': reset_indices,
        't_bound_lo'   : t_bound_lo,
        't_bound_hi'   : t_bound_hi,
    }

# ─── Main ─────────────────────────────────────────────────────────────────────
def DoFit():
    assert 1 <= len(DATA_FILES) <= 3, "DATA_FILES must list 1-3 filenames"
    n_files = len(DATA_FILES)
    
    print(f"Loading {n_files} file(s)...")
    files = [LoadFile(f) for f in DATA_FILES]

    V_thresh_estimate = float(np.mean([f['V_thresh'] for f in files]))
    
    V_thresh_estimate = 0
    for f in files:
        V_thresh_estimate = max(V_thresh_estimate, f['V_mem_ds'].max())
    
    t_pres = [0.000, 0]
    
    objective = MultiObjective(
        times     = [f['time_ds']  for f in files],
        V_mems    = [f['V_mem_ds'] for f in files],
        fit_masks = [f['fit_mask'] for f in files],
        V_thresh_estimate = V_thresh_estimate,
        t_pres = t_pres,
        drop_indices = [f['drop_indices'] for f in files]
    )
    
    if True:
        bounds = [
            (0.01, 3.0),
            (MEASURED_TAU_MEM - 0.005, MEASURED_TAU_MEM + 0.005),
        ]
        
        for f in files:
            bounds += [
                (0.01, 3.0)
            ]
    
        n_params = len(bounds)
        print(f"\nFitting {n_params} parameters "
              f"(3 shared + 2 × {n_files} per-file)...")
    
        t0 = time_lib.time()
    
        print("Running global search (differential_evolution)...")
        global_result = differential_evolution(
            objective, bounds,
            seed    = 52,
            maxiter = 1000,
            popsize = 20,
            tol     = 1e-9,
            polish  = False,
            workers = -1,
            init    = 'sobol',
        )
        print(f"Global:  score = {global_result.fun:.6f}  ({time_lib.time()-t0:.1f}s)")
    
        print("Running local refinement (Nelder-Mead)...")
        local_result = minimize(
            objective, global_result.x, method='Nelder-Mead',
            options={'maxiter': 20000, 'xatol': 1e-8, 'fatol': 1e-12}
        )
    
        p = local_result.x
        tau_syn_fit = p[0]
        tau_mem_fit = p[1]
        V_thresh_fit = V_thresh_estimate
        per_file = [p[2 + i] for i in range(n_files)]
        
        if not ADD_TAU_MEM_PARAM:
            tau_mem_fit = MEASURED_TAU_MEM
    
        elapsed = time_lib.time() - t0
        print(f"Total time  = {elapsed:.1f} s")
        print(f"\nTotal score = {local_result.fun:.8f}")
        score = local_result.fun
    else:
        score = 0
        tau_syn_fit = 0.1531
        tau_mem_fit = MEASURED_TAU_MEM
        per_file = [0.226, 1.146]
        V_thresh_fit = V_thresh_estimate
        
    print("\nShared parameters:")
    print(f"  tau_syn  = {tau_syn_fit  * 1000:.2f} ms")
    print(f"  tau_mem  = {tau_mem_fit  * 1000:.2f} ms")
    print(f"  V_thresh = {V_thresh_fit * 1000:.1f}  mV")
    for i, w in enumerate(per_file):
        print(f"\nFile {i+1} ({DATA_FILES[i]}):")
        print(f"  weight = {w:.4f} V")
    
    colors = ['steelblue', 'seagreen', 'mediumpurple']
    fig, ax = plt.subplots(figsize=(12, 6))

    for i, (f, w) in enumerate(zip(files, per_file)):
        V_sim, drop_indices_sim = SimulateLIF(f['time_full'], w, tau_syn_fit, t_pres[i], V_thresh_fit, tau_mem_fit)
        label = f['filename'].replace('.csv', '')
        col = colors[i]
        ax.plot(f['time_full'] * 1000, f['V_mem_full'], alpha=0.6, linewidth=0.8, color=col, label=f'Data  {label}')
        ax.plot(f['time_full'] * 1000, V_sim, linewidth=1.8, linestyle='--', color=col, label=f'Fit   {label}')
        dt = np.diff(f['time_full']).mean()
        for di, ri in zip(f['drop_indices'], f['reset_indices']):
            lo = f['time_full'][max(0, di - 2)] * 1000
            hi = f['time_full'][min(ri + int(0.005/dt), len(f['time_full'])-1)] * 1000
            ax.axvspan(lo, hi, alpha=0.08, color=col)

    ax.axhline(V_thresh_fit, color='red',  linestyle='--', linewidth=0.8, label=f'$V_{{thresh}}$ = {V_thresh_fit:.3f} V')
    ax.axhline(V_LEAK, color='gray', linestyle='--', linewidth=0.8, label=f'$V_{{leak}}$ = {V_LEAK:.3f} V')
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('$V_{mem}$ (V)')
    weight_str = '   '.join([f'$w_{{{i+1}}}$={w:.3f}V' for i, w in enumerate(per_file)])
    ax.set_title(
        f'Score: {score:.3f}   '
        f'$\\tau_{{syn}}$={tau_syn_fit*1000:.1f}ms   '
        f'$\\tau_{{mem}}$={tau_mem_fit*1000:.1f}ms   '
        f'$V_{{thresh}}$={V_thresh_fit*1000:.1f}mV   ' + weight_str
    )
    ax.legend(fontsize=8, ncol=2)
    plt.tight_layout()
    plt.savefig('Fit.png', dpi=400)
    plt.show()

if __name__ == '__main__':
    DoFit()