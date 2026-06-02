#pragma once

#include "ERP.h"

static constexpr std::array LAYER_SIZES = { 5u, 3u, 1u };

static constexpr uint32_t LAYER_COUNT = static_cast<uint32_t>(LAYER_SIZES.size());
static constexpr uint32_t INPUT_COUNT = LAYER_SIZES.front();
static constexpr uint32_t OUTPUT_COUNT = LAYER_SIZES.back();
static constexpr uint32_t HIDDEN_LAYERS = LAYER_COUNT - 2;

static constexpr Scalar WEIGHT_LIMIT = Scalar(100.0);

namespace Detail
{
    constexpr uint32_t WeightCount()
    {
        uint32_t total = 0;
        for (uint32_t l = 1; l < LAYER_COUNT; ++l)
            total += LAYER_SIZES[l - 1] * LAYER_SIZES[l];
        return total;
    }

    constexpr uint32_t BiasCount()
    {
        uint32_t total = 0;
        for (uint32_t l = 1; l < LAYER_COUNT; ++l)
            total += LAYER_SIZES[l];
        return total;
    }

    constexpr uint32_t MaxLayerSize()
    {
        uint32_t m = 0;
        for (uint32_t l = 0; l < LAYER_COUNT; ++l)
            m = (LAYER_SIZES[l] > m) ? LAYER_SIZES[l] : m;
        return m;
    }
}

static constexpr uint32_t WEIGHT_COUNT = Detail::WeightCount();
static constexpr uint32_t BIAS_COUNT = Detail::BiasCount();
static constexpr uint32_t MAX_LAYER_SIZE = Detail::MaxLayerSize();

struct NetworkGenome
{
    NetworkGenome()
    {
        for (auto& sigma : WeightsSigma)
            sigma = INITIAL_SIGMA;

        for (auto& sigma : BiasesSigma)
            sigma = INITIAL_SIGMA;
    }

    std::array<Scalar, WEIGHT_COUNT> Weights = {};
    std::array<Scalar, BIAS_COUNT> Biases = {};
    std::array<Scalar, WEIGHT_COUNT> WeightsSigma = {};
    std::array<Scalar, BIAS_COUNT> BiasesSigma = {};

    void Randomize(std::mt19937& rng, Scalar range = Scalar(1.0));
    void Mutate(std::mt19937& rng);
};

NetworkGenome Crossover(const NetworkGenome& a, const NetworkGenome& b, std::mt19937& rng);

class ContinuousNetwork
{
public:
    ContinuousNetwork() = default;
    explicit ContinuousNetwork(const NetworkGenome& genome) { SetFromGenome(genome); }

    void SetFromGenome(const NetworkGenome& genome);

    void Evaluate(const std::array<Scalar, INPUT_COUNT>& input, std::array<Scalar, OUTPUT_COUNT>& output) const;

    std::array<Scalar, OUTPUT_COUNT> Evaluate(const std::array<Scalar, INPUT_COUNT>& input) const;

    std::array<Scalar, WEIGHT_COUNT>& Weights() { return m_Weights; }
    std::array<Scalar, BIAS_COUNT>& Biases() { return m_Biases; }

private:
    static Scalar Activation(Scalar x);
    static Scalar OutputActivation(Scalar x);

    std::array<Scalar, WEIGHT_COUNT> m_Weights = {};
    std::array<Scalar, BIAS_COUNT> m_Biases = {};
};
