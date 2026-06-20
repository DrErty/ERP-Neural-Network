# Introduction

The leaky integrate-and-fire model (LIF) offers an energy-efficient
method for neuromorphic computing. We study control on Lu.i, a
palm-sized analog PCB implementation of LIF neurons. Unlike simulation,
these physical neurons introduce noise, device mismatch, and limited
precision. These constraints make cart-pole stabilization a useful
benchmark: the system is nonlinear, open-loop unstable, and requires
fast closed-loop feedback.

Previous work used near-linear LIF dynamics for LQR-style control
[@Banerjee_2025], often requiring neuron populations to obtain accurate
rate-based representations. Here, we instead avoid precise firing-rate
estimation. We treat threshold crossings around zero as sign activations
and use a compact controller of four physical analog LIF neurons: the
three state inputs are frequency-encoded directly, so the network
comprises three hidden neurons and one output neuron.

## Research Goal

Our goal is to test whether this minimal hardware network can stabilize
the cart-pole in near real time, and to characterize the calibration
needed to transfer trained weights onto noisy Lu.i hardware.

# Theory

- **Cart-Pole.** The plant is the standard simulated cart-pole: a
  point-mass pole ($m$, length $\ell$) hinged to a cart ($M$) on a
  horizontal rail and driven by a horizontal force. The angle $\theta$
  is measured from the upward vertical, so $\theta = 0$ is the unstable
  upright equilibrium and $\theta = \pm\pi$ the hanging rest state; the
  goal is to reach and hold $\theta = 0$, both from a small offset
  $\epsilon$ (stabilisation) and from hanging (swing-up). The force
  takes two levels, $F = \pm F_{\max}$ push left/right (bang-bang
  control), that is the output of a sign network and keeps training and
  wiring simple. The controller reads only $\theta$, $\dot\theta$ and
  $\dot x$, never the cart position. The point mass dynamics
  (frictionless rail) are $$\begin{align*}
    \ddot x &= \frac{F + m\ell\dot\theta^{2}\sin\theta - mg\sin\theta\cos\theta}
                    {M + m\sin^{2}\theta} \\
    \ddot\theta &= \frac{(M+m)g\sin\theta
                    - \cos\theta\,(F + m\ell\dot\theta^{2}\sin\theta)}
                    {\ell\,(M + m\sin^{2}\theta)}
  \end{align*}$$

- **Lu.i Neurons.** Each neuron is a palm-sized analog leaky
  integrate-and-fire (LIF) cell [@STRADMANN2025100248]: current-based
  synaptic inputs are integrated on a leaky membrane, and when the
  membrane crosses its threshold the neuron fires a spike and resets.
  Its three synapses have potentiometer-set weights with selectable sign
  (excitatory/inhibitory). Because information is carried in the spike
  rate, we encode each state variable as an input frequency and read the
  controller's decision from the output neuron's rate.

- **Controller.** The controller is a compact 3-3-1 feed-forward network
  of these neurons with $\mathrm{sign}(\cdot)$ activation; the output
  sign sets the force direction, $F = F_{\max}\,z_{\text{out}}$.

# Methods

- **Simulation.** The cart-pole equations of motion were integrated in a
  custom simulator with explicit (forward) RK4 at a fixed step
  $\Delta t = 1/60\,\mathrm{s}$ over a $60\,\mathrm{s}$ time period; the
  step was halved relative to our initial value to accommodate the
  finite response latency of the physical network. The same simulator
  was used both to obtain the controller offline and to evaluate the
  hardware-transferred policy in closed loop. We used the following
  standard constants in our simulation.

  ::: tabular
  @ l l R l @ **Symbol** & **Description** & **Value** & **Unit**\
  \
  $M$ & Cart mass & 1.0 & kg\
  $m$ & Pole point mass & 0.1 & kg\
  $\ell$ & Pole length & 0.5 & m\
  $g$ & Gravitational acceleration & 9.81 & m s^−2^\
  \
  $F_{\mathrm{max}}$ & Max applied horizontal force & 12 & N\
  \
  $\Delta t$ & Integration time step & 1/60 & s\
  $t_f$ & Final time & 60 & s\
  $N$ & Number of steps ($t_f/\Delta t$)& 3600 & ---\
  & Integration scheme & RK4 & ---\
  :::

- **Frequency encoding.** Communication between neurons is rate-based,
  so every signal is carried as a spike frequency. Each state variable
  $z_{\text{in},i}$ is mapped to an input frequency about a fixed
  offset,
  $$f_{\text{in},i} = f_{\text{offset},i} + f_{\text{scale},i}\,z_{\text{in},i}.$$
  A first-order Taylor expansion of a neuron's frequency transfer
  function about that offset,
  $$f_{\text{out}} \approx f_{\text{baseline}}
      + \sum_i w_i\,\big(f_{\text{in},i}-f_{\text{offset},i}\big),$$
  defines the effective weight $w_i$ as the local sensitivity of the
  output rate to input $i$, and the baseline
  $f_{\text{baseline}} = f_{\text{out}}(f_{\text{offset},i})$ as the
  output at the operating point, the latter being directly measurable.
  The binary output follows from thresholding this rate against its
  baseline,
  $$z_{\text{out}} = \mathrm{sign}\!\big(f_{\text{out}} - f_{\text{baseline}} + f_{\text{bias}}\big)
      \approx \mathrm{sign}\!\Big(f_{\text{scale}}\sum_i w_i z_i + f_{\text{bias}}\Big),$$
  where $f_{\text{out}}$ is sampled by an Arduino every
  $5\,\mathrm{ms}$. Hidden-layer outputs are re-encoded as frequencies
  and propagated; the output neuron's decision is converted to the force
  command and returned to the simulator.

- **Controller.** The controller is a 3-3-1 feed-forward network with
  $\mathrm{sign}(\cdot)$ activations [@Schmidhuber_2015], mapping the
  inputs $(\theta,\dot\theta,\dot x)$ to one signed output that is
  applied as a fixed-magnitude bang-bang force,
  $F = F_{\max}\,z_{\text{out}}$ (push left for $-1$, right for $+1$).

  Its weights and biases were obtained offline in our cart-pole
  simulator. Because the $\mathrm{sign}(\cdot)$ activation is
  non-differentiable, the parameters were optimized with an evolutionary
  strategy [@eiben2015introduction] using a fitness function that
  rewards proximity to the cart-pole's target energy (energy referenced
  to pivot height, so the upright state corresponds to
  $E_{\mathrm{target}} = mg\ell$). More precisely, the reward is
  $$E_{\mathrm{target}}=mg\ell, \quad
  E_{\max}=2mg\ell, \quad
  \text{reward}=1-\min\left(1,\frac{\left|E_{\mathrm{pole}} - E_{\text{target}}\right|}{E_{\max}}\right).$$
  Where $E_{\mathrm{pole}}$ is the energy of the pole in the simulator.

  Then, the weights are reproduced on hardware: each synaptic weight was
  set by tuning the corresponding Lu.i potentiometer until the measured
  weight matched the simulated value. Biases were implemented digitally
  on the Arduino, as the Lu.i neurons offered no direct mechanism for
  setting a bias.

# Results

The following results are all obtained from the physical Lu.i controller
described in methods:

<figure id="fig:results">
<figure>
<span class="image placeholder"
data-original-image-src="swingup_phase_portrait"
data-original-image-title="" width="\linewidth"></span>
<figcaption>Phase portrait of cart-pole starting in hanging state,
controller swings up pendulum, and is caught near <span
class="math inline"><em>θ</em> = 0</span>, settles into small bang-bang
limit cycle.</figcaption>
</figure>
<figure>
<span class="image placeholder"
data-original-image-src="swingup_theta_vs_time"
data-original-image-title="" width="\linewidth"></span>
<figcaption><span class="math inline"><em>θ</em></span> versus time
graph of cart-pole starting in hanging state, controller swings up
pendulum, and is caught near <span
class="math inline"><em>θ</em> = 0</span>, settles into small bang-bang
limit cycle.</figcaption>
</figure>
<figure>
<span class="image placeholder" data-original-image-src="theta_vs_time"
data-original-image-title="" width="\linewidth"></span>
<figcaption><span class="math inline"><em>θ</em></span> versus time
graph of cart-pole starting at a small offset from <span
class="math inline"><em>θ</em> = 0</span> (see legend), and is
stabilized by the controller near <span
class="math inline"><em>θ</em> = 0</span>, settles into small bang-bang
limit cycle.</figcaption>
</figure>
<figure>
<span class="image placeholder" data-original-image-src="stability_plot"
data-original-image-title="" width="\linewidth"></span>
<figcaption>The settling time, which described how long the controller
takes to bring the pendulum near <span
class="math inline"><em>θ</em> = 0</span> within a certain radius <span
class="math inline"><em>ϵ</em></span>.</figcaption>
</figure>
<figcaption>Closed-loop behavior of the physical Lu.i controller:
(a)–(b) swing-up from the hanging state; (c)–(d) stabilization from
small initial offsets.</figcaption>
</figure>

# Discussion and Interpretation

The controller solves both tasks using only three inputs and an on-off
output. The swing-up phase portrait (a) shows the network pumping energy
(swinging back and forth) until the pole is captured at the upright
equilibrium, where it holds within a small bang-bang limit cycle; the
corresponding time trace (b) reaches upright in about 5s. From small
initial offsets (c) the pole damps back to vertical, with settling time
increasing with offset magnitude (d). The residual jitter and the
scatter in settling time are intrinsic to discrete on-off actuation and
the finite latency of the analog neurons.

Our methods have a few key advantages over other models that you can see
from the results: Our choice of sign activation reduces noise in the
swing-up stage: earlier models using smooth activations such as
$\tanh(\cdot)$ were prone to noise during swing-up, almost always
undershooting and failing to bring the pendulum up. Unfortunately, this
comes with the downside of small oscillations around $\theta = 0$ once
the pendulum is stabilized (see figure (a)).

# Reflection & Future work

- We did not quantify the discrepancy between simulated and hardware
  (Lu.i) performance; future work could characterize how the two differ
  for each input state ($\theta, \dot{\theta}, \dot{x}$).

- Encoding scheme might not have been optimal for noise reduction, more
  research required.

- Some computations, such as applying the activation functions, are
  performed digitally on an Arduino; future work could implement them in
  an analog manner.

# Conclusion

A compact 3-3-1 sign-activation network, realized on physical Lu.i
neurons, both swings up and stabilizes the cart-pole from minimal
sensing and bang-bang force, demonstrating that low-cost analog LIF
hardware can implement a functional nonlinear controller.
