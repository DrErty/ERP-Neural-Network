#pragma once

#include "ERP.h"

static constexpr Scalar REFRAC_TIME = Scalar(13.0 / 1000.0); // Seconds
static constexpr Scalar PULSE_TIME = Scalar(12.0 / 1000.0);

static constexpr Scalar MAX_WEIGHT = Scalar(10.0);

static constexpr uint32_t MAX_INPUTS = 3;
static constexpr uint32_t MAX_OUTPUTS = 3;

static constexpr uint32_t INPUT_NEURON_COUNT = 3;
static constexpr uint32_t HIDDEN_NEURON_COUNT = 0;
static constexpr uint32_t OUTPUT_NEURON_COUNT = 1;
static constexpr uint32_t TOTAL_NEURON_COUNT = INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT;

using NeuronIdx = int8_t;

struct NeuronParams
{
    Scalar VDrive = Scalar(3.0);
    Scalar TauMem = Scalar(0.02);
    Scalar TauSyn = Scalar(0.02);
    Scalar VLeak = Scalar(0.5);
    Scalar VThreshold = Scalar(1.0);

    void Print() const;
};

class SpikeEncoder
{
public:
    SpikeEncoder(Scalar maxRate = MAX_INPUT_RATE, Scalar minRate = MIN_INPUT_RATE)
        : m_MaxRate(maxRate), m_MinRate(minRate), m_CurrentRate(Scalar(0.0)), m_Phase(Scalar(0.0)) {}

    void SetValue(Scalar value);
    void SetMaxRate(Scalar maxRate);
    void SetMinRate(Scalar minRate);

    uint32_t Update(Scalar dt);

    void Reset() { m_Phase = Scalar(0.0); }
    Scalar CurrentRate() const { return m_CurrentRate; }
    Scalar GetPhase() const { return m_Phase; }
private:
    Scalar m_MaxRate;
    Scalar m_MinRate;
    Scalar m_CurrentRate;
    Scalar m_Phase;
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
    static constexpr Scalar FREQUENCY_TIME_WINDOW = 0.5;

    std::array<Neuron, TOTAL_NEURON_COUNT> Neurons = {};

    std::array<std::array<Scalar, MAX_INPUTS>, TOTAL_NEURON_COUNT> Weights = {};
    std::array<std::array<Scalar, MAX_INPUTS>, TOTAL_NEURON_COUNT> I_in = {};

    std::array<Scalar, TOTAL_NEURON_COUNT> VMem = {};
    std::array<Scalar, TOTAL_NEURON_COUNT> RefracTime = {};
    std::array<Scalar, TOTAL_NEURON_COUNT> DecayRates = {};
    std::array<bool, TOTAL_NEURON_COUNT> InactiveNeurons = {};

    struct Trigger
    {
        Trigger() = delete;
        Trigger(Scalar time, NeuronIdx outputIdx) : Time(time), OutputIdx(outputIdx) {};

        Scalar Time;
        NeuronIdx OutputIdx;
    };

    std::vector<Trigger> RecentTriggers;

    Scalar GetFrequency(NeuronIdx outputIdx) const;

    void TriggerConnected(int8_t neuronIndex, uint32_t count, Scalar duration = PULSE_TIME);
    void UpdateNetwork(Scalar dt);

    void SetParams(uint32_t neuronIndex, const NeuronParams& params);
    const NeuronParams& GetParams(uint32_t neuronIndex) const;
private:
    std::array<NeuronParams, TOTAL_NEURON_COUNT> Params = {};
};

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, Scalar weight);