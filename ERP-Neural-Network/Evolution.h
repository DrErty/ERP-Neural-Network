#pragma once

#include "ERP.h"
#include "Neuron.h"
#include "FlappyBird.h"

static constexpr uint32_t MAX_INDIVIDUALS = 1000;
static constexpr uint32_t EVALUTIONS_PER_GENOME = 5;
static constexpr double INITIAL_SIGMA = 3.0;
static constexpr uint32_t TOTAL_WEIGHTS = INPUT_NEURONS * HIDDEN_NEURONS + HIDDEN_NEURONS * OUTPUT_NEURONS;

struct Range
{
    double Min = 0.0;
    double Max = 1.0;
};

struct Genome
{
    std::array<double, TOTAL_WEIGHTS> Weights = {};
    std::array<double, TOTAL_NEURONS> VLeaks = {};
    std::array<Range, 3> SpikeEncoderRange = {};

    void Print()
    {
        std::cout << "Weights: ";
        for (auto weight : Weights)
        {
            std::cout << weight << " ";
        }
        std::cout << std::endl;
        std::cout << "VLeaks: ";
        for (auto vLeak : VLeaks)
        {
            std::cout << vLeak << " ";
        }
        std::cout << std::endl;
        std::cout << "Ranges: ";
        for (auto range : SpikeEncoderRange)
        {
            std::cout << "Min: " << range.Min << ", Max: " << range.Max << " ";
        }
        std::cout << std::endl;
    }
};

struct Player
{
    NeuralNetwork Network = {};
    InputState InputState = {};
    uint32_t BirdIndex = 0;
};

struct Individual
{
    Genome Genome = {};
    std::array<Player, EVALUTIONS_PER_GENOME> Players = {};
    NeuralNetwork BaseNetwork = {};
    bool Alive = true;

    double EvaluateFitness(const FlappyBird& game) const
    {
        double totalFitness = 0.0;
        for (auto& player : Players)
        {
            const double fitness = game.BirdFitness(player.BirdIndex);
            totalFitness += fitness;
        }

        double lowestFitness = std::numeric_limits<double>::max();
        for (auto& player : Players)
        {
            const double fitness = game.BirdFitness(player.BirdIndex);
            if (fitness < lowestFitness)
                lowestFitness = fitness;
        }

        constexpr double alpha = 0.9;
        return lowestFitness * alpha + (1.0 - alpha) * totalFitness / static_cast<double>(EVALUTIONS_PER_GENOME);
    }
};

static void Mutate(Individual& individual, std::mt19937& rng, double sigma)
{
    std::normal_distribution<double> normalDistribution(0.0, sigma);
    std::normal_distribution<double> vLeakDistribution(0.0, sigma * 0.1);
    for (auto& weight : individual.Genome.Weights)
    {
        weight += normalDistribution(rng);
        weight = std::clamp(weight, -42.0, 42.0);
    }

    for (auto& vLeak : individual.Genome.VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }
#if 1
    for (auto& range : individual.Genome.SpikeEncoderRange)
    {
        range.Min += normalDistribution(rng);
        range.Max += normalDistribution(rng);

        if (range.Min > range.Max)
        {
            std::swap(range.Min, range.Max);
        }
        range.Min = std::round(range.Min);
        range.Max = std::round(range.Max);

        Assert(range.Min <= range.Max);
    }
#endif
}

static void ConstructNetwork(Individual& individual)
{
    uint32_t weightCount = 0;
    for (uint32_t i = 0; i < INPUT_NEURONS; i++)
    {
        for (uint32_t j = INPUT_NEURONS; j < INPUT_NEURONS + HIDDEN_NEURONS; j++)
        {
            ConnectNeurons(individual.BaseNetwork, j, i, individual.Genome.Weights[weightCount++]);
        }
    }

    for (uint32_t i = INPUT_NEURONS; i < INPUT_NEURONS + HIDDEN_NEURONS; i++)
    {
        for (uint32_t j = INPUT_NEURONS + HIDDEN_NEURONS; j < TOTAL_NEURONS; j++)
        {
            ConnectNeurons(individual.BaseNetwork, j, i, individual.Genome.Weights[weightCount++]);
        }
    }

    for (uint32_t neuronIndex = 0; neuronIndex < TOTAL_NEURONS; neuronIndex++)
    {
        individual.BaseNetwork.Neurons[neuronIndex].V_leak = individual.Genome.VLeaks[neuronIndex];
    }
}

struct NoisyEvalConfig
{
    float WeightNoiseSigma = 0.05f;
    float TauMemNoiseSigma = 0.05f;
    float TauSynNoiseSigma = 0.05f;
    float VThresholdNoiseSigma = 0.02f;
};

static void VaryNetwork(NeuralNetwork& network, std::mt19937& rng)
{
    NoisyEvalConfig config;

    std::normal_distribution<double> weightNoise(0.0f, config.WeightNoiseSigma);
    std::normal_distribution<double> tauMemNoise(0.0, config.TauMemNoiseSigma);
    std::normal_distribution<double> tauSynNoise(0.0, config.TauSynNoiseSigma);
    std::normal_distribution<double> vThresholdNoise(0.0, config.VThresholdNoiseSigma);

    for (Neuron& neuron : network.Neurons)
    {
        for (double& weight : neuron.Weights)
            weight *= (1.0 + weightNoise(rng));
        neuron.Tau_mem *= (1.0 + tauMemNoise(rng));
        neuron.Tau_syn *= (1.0 + tauSynNoise(rng));
        neuron.V_threshold *= (1.0 + vThresholdNoise(rng));
    }
}