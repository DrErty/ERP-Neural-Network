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
    odr_res.pprint()
    
    #print(f'Red Chi Squared = {odr_res.res_var}')
    
    return odr_res

def DoFit():
    data = np.genfromtxt('MetingJune1.csv', delimiter=",")
    
    V_mem = data[:,1]
    time = data[:,0]
    
    V_leak = V_mem[len(V_mem) - 1] #V
    
    #V_leak = 0.635
    
    mask = V_mem >= V_leak * 0.75
    
    V_mem = V_mem[mask]
    time = time[mask]
    
    begin_time = time[0]
    
    def model(B, t):
        return (V_mem[0] - V_leak) * np.exp(-(t - begin_time) / B[0]) + V_leak
    
    beta_0 = [1]
    
    odr_res = RunODR(time, V_mem, beta_0, model)
    print(odr_res.beta)
    print(odr_res.sd_beta)
    
    model_V_mem = model(odr_res.beta, time)
    
    plt.figure()
    plt.plot(time, V_mem, label = 'Data')
    plt.plot(time, model_V_mem, label = 'Model')
    plt.xlabel('Tijd (s)')
    plt.ylabel(r'$V_{mem}$ (V)')
    plt.savefig('Fit.png', dpi = 400)
    plt.legend()
    plt.show()
    
    print(V_leak)
    

def main():
    DoFit()
    
main()