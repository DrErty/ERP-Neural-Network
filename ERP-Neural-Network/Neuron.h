#pragma once

#include "ERP.h"

static constexpr double V_DD = 3; // Volt;
//static constexpr double V_REFRAC_START = V_DD / 4.0; // Volt
static constexpr double REFRAC_TIME = 12 / 1000.0; // Seconds

static constexpr uint32_t MAX_INPUTS = 3;
static constexpr uint32_t MAX_OUTPUTS = 3;

static constexpr uint32_t INPUT_NEURONS = 3;
static constexpr uint32_t HIDDEN_NEURONS = 3;
static constexpr uint32_t OUTPUT_NEURONS = 1;
static constexpr uint32_t TOTAL_NEURONS = INPUT_NEURONS + HIDDEN_NEURONS + OUTPUT_NEURONS;

class SpikeEncoder
{
public:
    SpikeEncoder(float maxRate, float minRate)
        : m_MaxRate(maxRate)
        , m_MinRate(minRate)
        , m_CurrentRate(0.0f)
        , m_Phase(0.0f)
    {}

    void SetValue(float value, float valueMin, float valueMax)
    {
        const float normalised = std::clamp((value - valueMin) / (valueMax - valueMin), 0.0f, 1.0f);
        m_CurrentRate = m_MinRate + normalised * (m_MaxRate - m_MinRate);
    }

    bool Update(float dt)
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

    void Reset() { m_Phase = 0.0f; }
    float CurrentRate() const { return m_CurrentRate; }
    float GetPhase() const { return m_Phase; }
private:
    float m_MaxRate;
    float m_MinRate;
    float m_CurrentRate;
    float m_Phase;
};

struct Neuron
{
    std::array<double, MAX_INPUTS> Weights = {}; // Volt
    std::array<double, MAX_INPUTS> I_in = {}; // 0 to 1

    double V_mem = 0.0; // Volt
    double Tau_mem = 0.73; // Seconds
    double Tau_syn = 0.07; // Seconds
    double V_leak = 0.656; // Volt
    double V_threshold = V_DD / 2.0 * (1.3 / 1.5); // Volt

    double RefracTime = 0.0;

    std::array<int8_t, MAX_OUTPUTS> OutputConnections = { -1, -1, -1 };
    std::array<int8_t, MAX_INPUTS> InputConnections = { -1, -1, -1 };
};

struct NeuralNetwork
{
    std::array<Neuron, TOTAL_NEURONS> Neurons = {};
};

struct InputState
{
    SpikeEncoder GapDistancePositive = { 1.0 / REFRAC_TIME, 0.0f };
    SpikeEncoder GapDistanceNegative = { 1.0 / REFRAC_TIME, 0.0f };
    SpikeEncoder VerticalVelocity = { 1.0 / REFRAC_TIME, 0.0f };
};

bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, double weight);

//static void UpdateNetwork(NeuralNetwork& network, std::array<ScopeBuffer, SCOPE_COUNT>& scopes, const std::array<uint32_t, SCOPE_COUNT>& scopeNeuronIndices)
std::vector<uint8_t> UpdateNetwork(NeuralNetwork& network, double dt);