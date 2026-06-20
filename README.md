# Introduction

The leaky integrate-and-fire model offers an energy-efficient method for
neuromorphic computing \[1\]\[2\]. The balancing cartpole is a canonical
benchmark for neuromorphic computing. Prior research has adressed
cartpoles starting in the upright position using few neurons. However,
behaviour of larger networks controlling cartpoles starting in the
bottom position (swing-up) is still unknown.

## Research Goal

Can four analog LIF neurons implement an effective controller that
stabilizes a cart-pole around its upright equilibrium, and can it
perform swing-ups?

# Theory

- **Cart-Pole.** The plant is the standard simulated cart-pole: a
  point-mass pole ($m$, length $\ell$) hinged to a cart ($M$) on a
  horizontal rail and driven by a horizontal force. The angle $\theta$
  is measured from the upward vertical, so $\theta = 0$ is the unstable
  upright equilibrium and $\theta = \pm\pi$ the hanging rest state; the
  goal is to reach and hold $\theta = 0$, both from a small offset
  $\epsilon$ (stabilisation) and from hanging (swing--up). The force
  takes two levels, $F = \pm F_{\max}$ (push left/right) bang-bang
  control [@BangBang] that is the natural output of a sign network and
  keeps training and wiring simple. The controller reads only $\theta$,
  $\dot\theta$ and $\dot x$, never the cart position. The point mass
  dynamics (frictionless rail) are $$\begin{align*}
    \ddot x &= \frac{F + m\ell\dot\theta^{2}\sin\theta - mg\sin\theta\cos\theta}
                    {M + m\sin^{2}\theta} \\
    \ddot\theta &= \frac{(M+m)g\sin\theta
                    - \cos\theta\,(F + m\ell\dot\theta^{2}\sin\theta)}
                    {\ell\,(M + m\sin^{2}\theta)}
  \end{align*}$$

- **Lu.i Neurons.** Each neuron is a palm--sized analog leaky integrate
  and fire (LIF) cell [@Lui]: current based synaptic inputs are
  integrated on a leaky membrane, and when the membrane crosses its
  threshold the neuron fires a spike and resets. Its three synapses have
  potentiometer--set weights with selectable sign
  (excitatory/inhibitory). Because information is carried in the spike
  rate, we encode each state variable as an input frequency and read the
  controller's decision from the output neuron's rate.

- **Controller.** The controller is a compact $3$--$3$--$1$
  feed--forward network of these neurons with
  $\operatorname{sign}(\cdot)$ activation; the output sign sets the
  force direction, $F = F_{\max}\,z_{\text{out}}$.

# Methods

- **Simulation.** The cart-pole equations of motion were integrated in a
  custom simulator with explicit (forward) RK4 at a fixed step
  $\Delta t = 1/60\,\mathrm{s}$ over a $60\,\mathrm{s}$ time period; the
  step was halved relative to our initial value to accommodate the
  finite response latency of the physical network. The same simulator
  was used both to obtain the controller offline and to evaluate the
  hardware-transferred policy in closed loop.

- **Controller.** The policy is a $3$-$3$-$1$ feed-forward network with
  $\operatorname{sign}(\cdot)$ activations, mapping the inputs
  $(\theta,\dot\theta,\dot x)$ to one signed output that is applied as a
  fixed-magnitude bang-bang force, $F = F_{\max}\,z_{\text{out}}$ (push
  left for $-1$, right for $+1$). Its weights and biases were obtained
  offline in the simulator and then reproduced on hardware: each
  synaptic weight was set by tuning the corresponding Lu.i potentiometer
  until the measured weight matched the simulated value.

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
  $$z_{\text{out}} = \operatorname{sign}\!\big(f_{\text{out}} - f_{\text{baseline}} + f_{\text{bias}}\big)
      \approx \operatorname{sign}\!\Big(f_{\text{scale}}\sum_i w_i z_i + f_{\text{bias}}\Big),$$
  where $f_{\text{out}}$ is sampled by an Arduino every
  $5\,\mathrm{ms}$. Hidden-layer outputs are re-encoded as frequencies
  and propagated; the output neuron's decision is converted to the force
  command and returned to the simulator.

# Results

<figure>
<figure>
<span class="image placeholder" data-original-image-src="theta_vs_time"
data-original-image-title="" width="\linewidth"></span>
<figcaption><strong>Schnitt</strong>: <span
class="math inline"><em>A</em> ∪ <em>B</em></span>: Element liegt in
<span class="math inline"><em>A</em></span> <strong>oder</strong> in
<span class="math inline"><em>B</em></span>.</figcaption>
</figure>
<figure>
<span class="image placeholder" data-original-image-src="stability_plot"
data-original-image-title="" width="\linewidth"></span>
<figcaption><strong>Vereinigung</strong>: <span
class="math inline"><em>A</em> ∩ <em>B</em></span>: Element liegt in
<span class="math inline"><em>A</em></span> <strong>und</strong> in
<span class="math inline"><em>B</em></span>.</figcaption>
</figure>
<figure>
<span class="image placeholder"
data-original-image-src="swingup_phase_portrait"
data-original-image-title="" width="\linewidth"></span>
<figcaption>Cart-pole starts hanging, controller swings up pendulum, and
is caught near <span class="math inline"><em>θ</em> = 0</span>, settles
into small bang-bang limit cycle.</figcaption>
</figure>
<figure>
<span class="image placeholder"
data-original-image-src="swingup_theta_vs_time"
data-original-image-title="" width="\linewidth"></span>
<figcaption><strong>Symmetrische Differenz</strong>: <span
class="math inline"><em>A</em><em>Δ</em><em>B</em></span>: Element liegt
<strong>entweder</strong> in <span class="math inline"><em>A</em></span>
oder in <span class="math inline"><em>B</em></span>.</figcaption>
</figure>
<figcaption><strong>Symmetrische Differenz</strong>: <span
class="math inline"><em>A</em><em>Δ</em><em>B</em></span>: Element liegt
<strong>entweder</strong> in <span class="math inline"><em>A</em></span>
oder in <span class="math inline"><em>B</em></span>.</figcaption>
</figure>

# Discussion

The controller solves both tasks using only three inputs and an on-off
output. The swing-up phase portrait (a) shows the network pumping energy
until the pole is captured at the upright equilibrium, where it holds
within a small bang-bang limit cycle; the corresponding time trace (b)
reaches upright in about 5s. From small initial offsets (c) the pole
damps back to vertical, with settling time increasing with offset
magnitude (d). The residual jitter and the scatter in settling time are
intrinsic to discrete on--off actuation and the finite latency of the
analog neurons.

# Conclusion & Reflection

A compact 3-3-1 sign-activation network, realised on physical Lu.i
neurons, both swings up and stabilises the cart-pole from minimal
sensing and bang-bang force, howing that low-cost analog LIF hardware
can implement a functional nonlinear controller.
