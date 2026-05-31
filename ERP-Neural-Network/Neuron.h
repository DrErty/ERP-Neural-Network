#pragma once

#include "ERP.h"

static constexpr double REFRAC_TIME = 13.0 / 1000.0; // Seconds
static constexpr double PULSE_TIME = 12.0 / 1000.0;

static constexpr double MAX_WEIGHT = 1.0;

static constexpr uint32_t MAX_INPUTS = 3;
static constexpr uint32_t MAX_OUTPUTS = 3;

static constexpr uint32_t INPUT_NEURON_COUNT = 8;
static constexpr uint32_t HIDDEN_NEURON_COUNT = 6;
static constexpr uint32_t OUTPUT_NEURON_COUNT = 2;
static constexpr uint32_t TOTAL_NEURON_COUNT = INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT;

using NeuronIdx = int8_t;

struct NeuronParams
{
    double VDrive = 3.0;
    double TauMem = 0.02;
    double TauSyn = 0.02;
    double VLeak = 0.5;
    double VThreshold = 1.0;

    void Print() const;
};

class SpikeEncoder
{
public:
    SpikeEncoder(float maxRate = 2.0 / REFRAC_TIME, float minRate = 0.0f)
        : m_MaxRate(maxRate), m_MinRate(minRate), m_CurrentRate(0.0f), m_Phase(0.0f) {}

    void SetValue(float value, float valueMin, float valueMax);

    uint32_t Update(float dt);

    void Reset() { m_Phase = 0.0f; }
    float CurrentRate() const { return m_CurrentRate; }
    float GetPhase() const { return m_Phase; }
private:
    float m_MaxRate;
    float m_MinRate;
    float m_CurrentRate;
    float m_Phase;
};

NeuronIdx GetNeuronIdxComplement(NeuronIdx neuronIdx);
NeuronIdx GetNeuronIdxFromInputIdx(NeuronIdx inputIdx);
NeuronIdx GetNeuronIdxFromHiddenIdx(NeuronIdx hiddenIdx);
NeuronIdx GetNeuronIdxFromOutputIdx(NeuronIdx outputIdx);

struct Neuron
{
    Neuron()
    {
        for (int8_t& val : OutputConnections)
            val = -1;
        for (int8_t& val : InputConnections)
            val = -1;
        for (int8_t& val : OutputConnectionsSynapseIdx)
            val = -1;
    }

    std::array<int8_t, MAX_OUTPUTS> OutputConnections = {};
    std::array<int8_t, MAX_OUTPUTS> OutputConnectionsSynapseIdx = {};

    std::array<int8_t, MAX_INPUTS> InputConnections = {};
};

struct NeuralNetwork
{
    std::array<Neuron, TOTAL_NEURON_COUNT> Neurons = {};

    std::array<std::array<double, MAX_INPUTS>, TOTAL_NEURON_COUNT> Weights = {};
    std::array<std::array<double, MAX_INPUTS>, TOTAL_NEURON_COUNT> I_in = {};

    std::array<double, TOTAL_NEURON_COUNT> VMem = {};
    std::array<double, TOTAL_NEURON_COUNT> RefracTime = {};
    std::array<double, TOTAL_NEURON_COUNT> DecayRates = {};
    std::array<bool, TOTAL_NEURON_COUNT> PendingTrigger = {};
    std::array<bool, TOTAL_NEURON_COUNT> InactiveNeurons = {};

    void TriggerConnected(int8_t neuronIndex, uint32_t count, double duration = PULSE_TIME);
    void UpdateNetwork(double dt);

    void SetParams(uint32_t neuronIndex, const NeuronParams& params);
    const NeuronParams& GetParams(uint32_t neuronIndex) const;
private:
    std::array<NeuronParams, TOTAL_NEURON_COUNT> Params = {};
};

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, double weight);