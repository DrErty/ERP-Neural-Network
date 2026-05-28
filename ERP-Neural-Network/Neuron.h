#pragma once

#include "ERP.h"

static constexpr double REFRAC_TIME = 12 / 1000.0; // Seconds

static constexpr uint32_t MAX_INPUTS = 3;
static constexpr uint32_t MAX_OUTPUTS = 3;

static constexpr uint32_t INPUT_NEURON_COUNT = 8;
static constexpr uint32_t HIDDEN_NEURON_COUNT = 6;
static constexpr uint32_t OUTPUT_NEURON_COUNT = 2;
static constexpr uint32_t TOTAL_NEURON_COUNT = INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT;

struct NeuronParams
{
    double VDrive = 0.0;
    double TauMem = 0.0;
    double TauSyn = 0.0;
    double VLeak = 0.0;
    double VThreshold = 0.0;
};

class SpikeEncoder
{
public:
    SpikeEncoder(float maxRate = 1.0f / static_cast<float>(REFRAC_TIME), float minRate = 0.0f)
        : m_MaxRate(maxRate), m_MinRate(minRate), m_CurrentRate(0.0f), m_Phase(0.0f) {}

    void SetValue(float value, float valueMin, float valueMax);

    bool Update(float dt);

    void Reset() { m_Phase = 0.0f; }
    float CurrentRate() const { return m_CurrentRate; }
    float GetPhase() const { return m_Phase; }
private:
    float m_MaxRate;
    float m_MinRate;
    float m_CurrentRate;
    float m_Phase;
};

uint32_t GetNeuronIdxComplement(uint32_t neuronIdx);
uint32_t GetNeuronIdxFromInputIdx(uint32_t inputIdx);
uint32_t GetNeuronIdxFromHiddenIdx(uint32_t hiddenIdx);
uint32_t GetNeuronIdxFromOutputIdx(uint32_t outputIdx);

struct Neuron
{
    Neuron()
    {
        for (int8_t& val : OutputConnections)
            val = -1;
        for (int8_t& val : InputConnections)
            val = -1;
    }

    std::array<double, MAX_INPUTS> Weights = {}; // Volt
    std::array<double, MAX_INPUTS> I_in = {}; // 0 to 1

    double V_mem = 0.0; // Volt

    NeuronParams Params = {};

    double RefracTime = 0.0;

    std::array<int8_t, MAX_OUTPUTS> OutputConnections = {};
    std::array<int8_t, MAX_INPUTS> InputConnections = {};

    bool Inactive = false;
    bool PendingTrigger = false;
};

struct NeuralNetwork
{
    std::array<Neuron, TOTAL_NEURON_COUNT> Neurons = {};

    void TriggerConnected(int8_t neuronIndex);
};

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, double weight);

void UpdateNetwork(NeuralNetwork& network, double dt);