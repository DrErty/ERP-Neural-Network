"""
Created on Wed May 13 12:22:56 2026
Author: Vincent
"""

import scipy.odr as odr
import numpy as np
import matplotlib.pyplot as plt

def RunODR(x, y, beta_0, model, sig_x = None, sig_y = None):
    odr_model = odr.Model(model)
    odr_data = odr.RealData(x, y, sx = sig_x, sy = sig_y)
    odr_obj = odr.ODR(odr_data, odr_model, beta0 = beta_0)
    if type(sig_x) == type(None):
        odr_obj.set_job(fit_type = 2)
    odr_res = odr_obj.run()
    
    #odr_res.pprint()
    #print(f'Red Chi Squared = {odr_res.res_var}')
    
    return odr_res

def DoFit():
    data = np.genfromtxt('Data_Meting_2.csv', delimiter=",")
    
    mask = np.logical_and(data[:,0] > 5, data[:,0] < 5.5)
    
    V_mem = data[:,1][mask]
    time = data[:,0][mask]
    
    dt = np.diff(time)[0]
    
    V_leak = V_mem[0]
    V_thres = np.max(V_mem)
    
    dip_mask = np.logical_or(time < 5.085, time > 5.1)
    
    # Hardcoded values.
    # This was needed because ODR couldn't handle more than 3 degrees of freedom, and these quantities are easy to determine by eye.
    
    def model(B, t):
        tau_mem, tau_syn, weight, t_pre, refrac_period = (B)
        
        tau_mem = np.abs(tau_mem)
        tau_syn = np.abs(tau_syn)
        weight = np.abs(weight)
        
        V_mem_model = []
        
        V_mem_sim = V_leak
        last_fired = time[0] - 1000;
        for t_current in t:
            if (t_current < t_pre):
                I_syn = 0
            else:
                I_syn = weight * np.exp(-(t_current - t_pre) / tau_syn)
            
            dV = (V_leak - V_mem_sim + I_syn) / tau_mem * dt
            
            if ((t_current - last_fired) < refrac_period):
                dV = 0
            
            V_mem_sim += dV
            
            if (V_mem_sim > V_thres):
                last_fired = t_current
                V_mem_sim = 0
            
            V_mem_model.append(V_mem_sim)
            
        return np.array(V_mem_model)
    
    best_odr_res = None
    best_score = 1000
    
    trialCount = 100
    for trialIndex in range(trialCount):
        beta_0 = [np.random.uniform(0, 1), np.random.uniform(0, 0.1), np.random.uniform(0, 3), np.random.uniform(5.05, 5.15), np.random.uniform(10 / 1000, 15 / 1000)]
        
        odr_res = RunODR(time, V_mem, beta_0, model)
        #print(odr_res.beta)
        #print(odr_res.sd_beta)
        
        model_V_mem = model(odr_res.beta, time)
        score = np.sum((model_V_mem[dip_mask] - V_mem[dip_mask]) ** 2)
        #print(f'score: {score}')
        if (score < best_score):
            best_score = score
            best_odr_res = odr_res
        
        #if (trialIndex % (max(trialCount // 10, 1)) == 0):
        #    print(f'{trialIndex/trialCount * 100}%')
    
    print(best_odr_res.beta)
    print(best_odr_res.sd_beta)
    print(best_score)
    
    model_V_mem = model(best_odr_res.beta, time)
    
    plt.figure()
    plt.title(f'Fit, Score: {best_score:.3f}, Tau_mem = {best_odr_res.beta[0] * 1000:.1f}ms, Tau_syn = {best_odr_res.beta[1] * 1000:.1f}ms, w = {best_odr_res.beta[2]:.2f}V')
    plt.plot(time, V_mem, label = 'Data')
    plt.plot(time, model_V_mem, label = 'Model')
    #plt.xlim(5.08, 5.1)
    plt.xlabel('Tijd (s)')
    plt.ylabel(r'$V_{mem}$ (V)')
    plt.savefig('Fit.png', dpi = 400)
    plt.legend()
    plt.show()
    

def main():
    DoFit()
    
main()