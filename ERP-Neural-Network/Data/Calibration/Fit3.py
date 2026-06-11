import scipy.odr as odr
import numpy as np
import matplotlib.pyplot as plt
from scipy.ndimage import gaussian_filter1d

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
    data = np.genfromtxt('Meting.csv', delimiter=",")
    
    #V_drive = data[:,2]
    V_mem = data[:,1]
    time = data[:,0]
    
    #V_mem = gaussian_filter1d(V_mem, sigma=10)
    
    mask = time > 0
    
    V_mem = V_mem[mask]
    time = time[mask]
    
    tau_mem = 0.13725662
    V_leak = 0.662
    
    def model(B, t, debug = False):
        weight = B[0]
        tau_syn = B[1]
        t_pre = B[2]
        
        V_mem = ((weight * tau_syn) / (tau_mem - tau_syn)) * (np.exp(-(t - t_pre) / tau_mem) - np.exp(-(t - t_pre) / tau_syn)) + V_leak
        
        return V_mem
    
    beta_0 = [1, 0.05, 0]
    
    odr_res = RunODR(time, V_mem, beta_0, model)
    print(odr_res.beta)
    print(odr_res.sd_beta)
    
    model_V_mem = model(odr_res.beta, time, debug = False)
    
    plt.figure()
    plt.plot(time, model_V_mem, label = 'Model')
    plt.plot(time, V_mem, label = 'Data')
    plt.xlabel('Tijd (s)')
    plt.ylabel(r'$V_{mem}$ (V)')
    plt.savefig('Fit.png', dpi = 400)
    plt.legend()
    plt.show()
    

def main():
    DoFit()
    
main()