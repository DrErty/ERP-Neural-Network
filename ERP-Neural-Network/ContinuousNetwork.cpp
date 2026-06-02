#include "ContinuousNetwork.h"

#include <algorithm>
#include <cmath>
#include <utility>   // std::swap

void NetworkGenome::Randomize(std::mt19937& rng, Scalar range)
{
    std::uniform_real_distribution<Scalar> dist(-range, range);
    for (Scalar& w : Weights) w = dist(rng);
    for (Scalar& b : Biases)  b = dist(rng);
}

void NetworkGenome::Mutate(std::mt19937& rng)
{
    for (uint32_t idx = 0; idx < WEIGHT_COUNT; idx++)
    {
        std::normal_distribution<Scalar> step(Scalar(0.0), WeightsSigma[idx]);
        Scalar& w = Weights[idx];
        w = std::clamp(w + step(rng), -WEIGHT_LIMIT, WEIGHT_LIMIT);
    }
    for (uint32_t idx = 0; idx < BIAS_COUNT; idx++)
    {
        std::normal_distribution<Scalar> step(Scalar(0.0), BiasesSigma[idx]);
        Scalar& b = Biases[idx];
        b = std::clamp(b + step(rng), -WEIGHT_LIMIT, WEIGHT_LIMIT);
    }

    std::normal_distribution<double> n01(0.0, 1.0);

    const double n = static_cast<double>(std::max<size_t>(1, WEIGHT_COUNT));
    const double tauGlobal = 1.0 / std::sqrt(2.0 * n);
    const double tauLocal = 1.0 / std::sqrt(2.0 * std::sqrt(n));

    const double globalFactor = std::exp(tauGlobal * n01(rng));

    for (auto& sigma : WeightsSigma)
    {
        sigma *= globalFactor * std::exp(tauLocal * n01(rng));
        sigma = std::clamp(sigma, Scalar(0.0), INITIAL_SIGMA);
    }
    for (auto& sigma : BiasesSigma)
    {
        sigma *= globalFactor * std::exp(tauLocal * n01(rng));
        sigma = std::clamp(sigma, Scalar(0.0), INITIAL_SIGMA);
    }
}

NetworkGenome Crossover(const NetworkGenome& a, const NetworkGenome& b, std::mt19937& rng)
{
    std::uniform_int_distribution<int> distribution(0, 1);
    NetworkGenome child;
    for (uint32_t i = 0; i < WEIGHT_COUNT; ++i)
        child.Weights[i] = distribution(rng) ? a.Weights[i] : b.Weights[i];
    for (uint32_t i = 0; i < BIAS_COUNT; ++i)
        child.Biases[i] = distribution(rng) ? a.Biases[i] : b.Biases[i];
    return child;
}

void ContinuousNetwork::SetFromGenome(const NetworkGenome& genome)
{
    m_Weights = genome.Weights;
    m_Biases = genome.Biases;
}

Scalar ContinuousNetwork::Activation(Scalar x)
{
    return std::tanh(x);
}

Scalar ContinuousNetwork::OutputActivation(Scalar x)
{
    return std::tanh(x);
}

void ContinuousNetwork::Evaluate(const std::array<Scalar, INPUT_COUNT>& input, std::array<Scalar, OUTPUT_COUNT>& output) const
{
    std::array<Scalar, MAX_LAYER_SIZE> bufA{};
    std::array<Scalar, MAX_LAYER_SIZE> bufB{};

    Scalar* src = bufA.data();
    Scalar* dst = bufB.data();

    for (uint32_t i = 0; i < INPUT_COUNT; ++i)
        src[i] = input[i];

    uint32_t weightOffset = 0;
    uint32_t biasOffset = 0;

    for (uint32_t layer = 1; layer < LAYER_COUNT; ++layer)
    {
        const uint32_t prevSize = LAYER_SIZES[layer - 1];
        const uint32_t curSize = LAYER_SIZES[layer];
        const bool     isOutput = (layer == LAYER_COUNT - 1);

        for (uint32_t j = 0; j < curSize; ++j)
        {
            Scalar acc = m_Biases[biasOffset + j];
            const uint32_t row = weightOffset + j * prevSize;
            for (uint32_t i = 0; i < prevSize; ++i)
                acc += m_Weights[row + i] * src[i];

            dst[j] = isOutput ? OutputActivation(acc) : Activation(acc);
        }

        weightOffset += prevSize * curSize;
        biasOffset += curSize;

        std::swap(src, dst);
    }

    for (uint32_t o = 0; o < OUTPUT_COUNT; ++o)
        output[o] = src[o];
}

std::array<Scalar, OUTPUT_COUNT> ContinuousNetwork::Evaluate(const std::array<Scalar, INPUT_COUNT>& input) const
{
    std::array<Scalar, OUTPUT_COUNT> output{};
    Evaluate(input, output);
    return output;
}