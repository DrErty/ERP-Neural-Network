import os
import pandas as pd
import matplotlib.pyplot as plt

fig, axes = plt.subplots(5,2,figsize=(8.27,11.69),sharex=False,sharey=False)
axes = axes.flatten()

for i in range(1,11):
    filenameSim = f"SIM\\Meting{i}.txt"
    filenameExp = f"EXP\\Meting{i}.txt"
    ax = axes[i - 1]

    dataSim = pd.read_csv(filenameSim, sep=None, engine="python")
    dataExp = pd.read_csv(filenameExp, sep=None, engine="python")

    timeSim = dataSim.iloc[:,0]
    thetaSim = dataSim.iloc[:,1]
    
    timeExp = dataExp.iloc[:,0]
    thetaExp = dataExp.iloc[:,1]

    ax.plot(timeSim, thetaSim, linewidth=0.6, label = "Sim")
    ax.plot(timeExp, thetaExp, linewidth=0.6, label = "Exp")
    ax.set_title(f"Meting {i}", fontsize=8)
    ax.grid(True, linewidth=0.3)
    ax.tick_params(labelsize=7)

fig.supxlabel("Time", fontsize=10)
fig.supylabel("Theta", fontsize=10)
fig.suptitle("Theta vs Time", fontsize=12)
fig.legend()
fig.tight_layout()
fig.savefig("metingen_subplot.png", dpi=300, bbox_inches="tight")
plt.show()