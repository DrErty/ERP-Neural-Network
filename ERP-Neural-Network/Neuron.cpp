#include "Neuron.h"

void SpikeEncoder::SetValue(float value, float valueMin, float valueMax)
{
    const float normalised = std::clamp((value - valueMin) / (valueMax - valueMin), 0.0f, 1.0f);
    m_CurrentRate = m_MinRate + normalised * (m_MaxRate - m_MinRate);
}

bool SpikeEncoder::Update(float dt)
{
    if (m_CurrentRate <= 0.0f) return false;
    m_Phase += m_CurrentRate * dt;
    if (m_Phase >= 1.0f)
    {
        m_Phase -= 1.0f;
        return true;
    }
    return false;
}

uint32_t GetNeuronIdxComplement(uint32_t neuronIdx)
{
    if (neuronIdx < INPUT_NEURON_COUNT)
        return INPUT_NEURON_COUNT - neuronIdx - 1;
    else if ((neuronIdx >= INPUT_NEURON_COUNT) && (neuronIdx < (INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT)))
        return (HIDDEN_NEURON_COUNT - (neuronIdx - INPUT_NEURON_COUNT) - 1) + INPUT_NEURON_COUNT;
    else
        return (OUTPUT_NEURON_COUNT - (neuronIdx - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT) - 1) + INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT;
}

uint32_t GetNeuronIdxFromInputIdx(uint32_t inputIdx)
{
    Assert(inputIdx < INPUT_NEURON_COUNT);
    return inputIdx;
}

uint32_t GetNeuronIdxFromHiddenIdx(uint32_t hiddenIdx)
{
    Assert(hiddenIdx < HIDDEN_NEURON_COUNT);
    return hiddenIdx + INPUT_NEURON_COUNT;
}

uint32_t GetNeuronIdxFromOutputIdx(uint32_t outputIdx)
{
    Assert(outputIdx < OUTPUT_NEURON_COUNT);
    return outputIdx + INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT;
}

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, double weight)
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
    network.Neurons[inputNeuron].Weights[inputSyn] = weight;

    return true;
}

void UpdateNetwork(NeuralNetwork& network, double dt)
{
    for (uint32_t i = 0; i < TOTAL_NEURON_COUNT; i++)
    {
        Neuron& neuron = network.Neurons[i];
        if (neuron.Inactive) continue;

        double I_syn = 0.0;
        for (uint32_t j = 0; j < MAX_INPUTS; j++)
        {
            neuron.I_in[j] += (-neuron.I_in[j] / neuron.Params.TauSyn) * dt; // Expontential Decay
            I_syn += neuron.Weights[j] * neuron.I_in[j];
        }

        neuron.RefracTime -= dt;
        if (neuron.RefracTime <= 0.0)
        {
            const double dV = (-(neuron.V_mem - neuron.Params.VLeak) + I_syn) / neuron.Params.TauMem;
            neuron.V_mem += dV * dt;
            if (neuron.V_mem < 0.0)
            {
                neuron.V_mem = 0.0;
            }
        }

        // Check if should trigger
        if (neuron.V_mem >= neuron.Params.VThreshold)
        {
            network.TriggerConnected(i);
            neuron.V_mem = 0;
            neuron.RefracTime = REFRAC_TIME;
            neuron.PendingTrigger = true;
        }
    }
}

void NeuralNetwork::TriggerConnected(int8_t neuronIndex)
{
    Neuron& neuron = Neurons[neuronIndex];
    for (uint32_t j = 0; j < MAX_OUTPUTS; j++)
    {
        int8_t toIndex = neuron.OutputConnections[j];
        if (toIndex == -1) continue;

        int32_t inputSyn = -1;
        for (uint32_t k = 0; k < MAX_INPUTS; k++)
        {
            if (Neurons[toIndex].InputConnections[k] == neuronIndex)
                inputSyn = k;
        }
        Assert(inputSyn != -1);

        Neurons[toIndex].I_in[inputSyn] = 1.0;
    }
}
