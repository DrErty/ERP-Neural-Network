#include "Neuron.h"

void NeuronParams::Print() const
{
    ERP_LOG("Params: TauMem: ", TauMem, ", TauSyn: ", TauSyn);
}

void SpikeEncoder::SetValue(Scalar value)
{
    const Scalar normalised = std::clamp(value, Scalar(-1.0), Scalar(1.0));
    m_CurrentRate = m_MinRate + (normalised + Scalar(1.0)) / Scalar(2.0) * (m_MaxRate - m_MinRate);
}

void SpikeEncoder::SetMaxRate(Scalar maxRate)
{
    m_MaxRate = maxRate;
}

void SpikeEncoder::SetMinRate(Scalar minRate)
{
    m_MinRate = minRate;
}

uint32_t SpikeEncoder::Update(Scalar dt)
{
    if (m_CurrentRate <= Scalar(0.0)) return 0;
    m_Phase += m_CurrentRate * dt;

    const uint32_t rounded = static_cast<uint32_t>(m_Phase);
    m_Phase -= rounded;

    return rounded;
}

NeuronIdx GetNeuronIdxComplement(NeuronIdx neuronIdx)
{
    Assert(neuronIdx >= 0 and neuronIdx < TOTAL_NEURON_COUNT);
    if (neuronIdx < INPUT_NEURON_COUNT)
        return INPUT_NEURON_COUNT - neuronIdx - 1;
    else if ((neuronIdx >= INPUT_NEURON_COUNT) && (neuronIdx < (INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT)))
        return (HIDDEN_NEURON_COUNT - (neuronIdx - INPUT_NEURON_COUNT) - 1) + INPUT_NEURON_COUNT;
    else
        return (OUTPUT_NEURON_COUNT - (neuronIdx - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT) - 1) + INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT;
}

NeuronIdx GetNeuronIdxFromInputIdx(NeuronIdx inputIdx)
{
    Assert(inputIdx >= 0 and inputIdx < INPUT_NEURON_COUNT);
    return inputIdx;
}

NeuronIdx GetNeuronIdxFromHiddenIdx(NeuronIdx hiddenIdx)
{
    Assert(hiddenIdx >= 0 and hiddenIdx < HIDDEN_NEURON_COUNT);
    return hiddenIdx + INPUT_NEURON_COUNT;
}

NeuronIdx GetNeuronIdxFromOutputIdx(NeuronIdx outputIdx)
{
    Assert(outputIdx >= 0 and outputIdx < OUTPUT_NEURON_COUNT);
    return outputIdx + INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT;
}

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, Scalar weight)
{
    int32_t outputSyn = -1;
    for (uint32_t i = 0; i < MAX_OUTPUTS; i++)
    {
        if (network.Neurons[outputNeuron].OutputConnections[i] == inputNeuron)
            Assert(false);

        if (network.Neurons[outputNeuron].OutputConnections[i] == -1)
            outputSyn = i;
    }

    int32_t inputSyn = -1;
    for (uint32_t i = 0; i < MAX_INPUTS; i++)
    {
        if (network.Neurons[outputNeuron].InputConnections[i] == outputNeuron)
            Assert(false);

        if (network.Neurons[inputNeuron].InputConnections[i] == -1)
            inputSyn = i;
    }

    Assert(inputSyn != -1);
    Assert(outputSyn != -1);

    network.Neurons[outputNeuron].OutputConnections[outputSyn] = inputNeuron;
    network.Neurons[inputNeuron].InputConnections[inputSyn] = outputNeuron;
    network.Weights[inputNeuron][inputSyn] = weight;

    network.Neurons[outputNeuron].OutputConnectionsSynapseIdx[outputSyn] = inputSyn;

    return true;
}

Scalar NeuralNetwork::GetFrequency(NeuronIdx outputIdx) const
{
    uint32_t count = 0;
    for (auto& trigger : RecentTriggers)
    {
        if (trigger.OutputIdx == outputIdx)
            count++;
    }
    return count / FREQUENCY_TIME_WINDOW;
}

void NeuralNetwork::TriggerConnected(int8_t neuronIndex, uint32_t count, Scalar duration)
{
    Neuron& neuron = Neurons[neuronIndex];
    for (uint32_t j = 0; j < MAX_OUTPUTS; j++)
    {
        int8_t toIndex = neuron.OutputConnections[j];
        if (toIndex == -1) continue;

        int8_t inputSyn = neuron.OutputConnectionsSynapseIdx[j];

        Assert((duration / PULSE_TIME) * static_cast<Scalar>(count) >= Scalar(0.0));

        I_in[toIndex][inputSyn] += (duration / PULSE_TIME) * static_cast<Scalar>(count);
    }
}

void NeuralNetwork::UpdateNetwork(Scalar dt)
{
    // TODO: uses fact that triggers are ordered.
    uint32_t removeCount = 0;
    for (auto& trigger : RecentTriggers)
    {
        trigger.Time += dt;
        if (trigger.Time > FREQUENCY_TIME_WINDOW)
        {
            removeCount += 1;
        }
    }
    while (removeCount > 0)
    {
        RecentTriggers.erase(RecentTriggers.begin());
        removeCount--;
    }

    for (uint32_t neuronIdx = 0; neuronIdx < TOTAL_NEURON_COUNT; neuronIdx++)
    {
        if (InactiveNeurons[neuronIdx]) continue;

        Scalar I_syn = Scalar(0.0);
        for (uint32_t j = 0; j < MAX_INPUTS; j++)
        {
            I_in[neuronIdx][j] *= DecayRates[neuronIdx];
            I_syn += Weights[neuronIdx][j] * I_in[neuronIdx][j];
            //Assert(Weights[neuronIdx][j] * I_in[neuronIdx][j] >= Scalar(0.0));
        }

        RefracTime[neuronIdx] -= dt;
        if (RefracTime[neuronIdx] <= Scalar(0.0))
        {
            const Scalar dV = (-(VMem[neuronIdx] - Params[neuronIdx].VLeak) + I_syn) / Params[neuronIdx].TauMem;
            Assert(Params[neuronIdx].TauMem >= Scalar(0.0));
            Assert(Params[neuronIdx].VLeak >= Scalar(0.0));
            VMem[neuronIdx] += dV * dt;
            if (VMem[neuronIdx] < Scalar(0.0))
            {
                VMem[neuronIdx] = Scalar(0.0);
            }
        }

        // Check if should trigger
        if (VMem[neuronIdx] >= Params[neuronIdx].VThreshold)
        {
            TriggerConnected(neuronIdx, 1);
            VMem[neuronIdx] = Scalar(0.0);
            RefracTime[neuronIdx] = REFRAC_TIME;

            if (neuronIdx >= INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT)
            {
                NeuronIdx outputIdx = neuronIdx - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT;
                RecentTriggers.emplace_back(Scalar(0.0), outputIdx);
            }
        }
    }
}

void NeuralNetwork::SetParams(uint32_t neuronIndex, const NeuronParams& params)
{
    constexpr Scalar dt = SIM_DT / static_cast<Scalar>(NEURON_SUBSTEPS);

    Params[neuronIndex] = params;
    DecayRates[neuronIndex] = std::exp(-dt / params.TauSyn);
}

const NeuronParams& NeuralNetwork::GetParams(uint32_t neuronIndex) const
{
    return Params[neuronIndex];
}
