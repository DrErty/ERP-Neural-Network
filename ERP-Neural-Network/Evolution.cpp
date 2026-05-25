#include "Evolution.h"

void Genome::Print()
{
    std::cout << "Weights: ";
    for (double weight : Weights)
    {
        std::cout << weight << " ";
    }
    std::cout << std::endl;
    std::cout << "VLeaks: ";
    for (double vLeak : VLeaks)
    {
        std::cout << vLeak << " ";
    }
    std::cout << std::endl;
    std::cout << "Ranges: ";
    for (const Range& range : SpikeEncoderRange)
    {
        std::cout << "Min: " << range.Min << ", Max: " << range.Max << " ";
    }
    std::cout << std::endl;
}

void Genome::Mutate(std::mt19937& rng, double sigma)
{
    std::normal_distribution<double> normalDistribution(0.0, sigma);
    std::normal_distribution<double> vLeakDistribution(0.0, sigma * 0.1);

    for (double& weight : Weights)
    {
        weight += normalDistribution(rng);
        weight = std::clamp(weight, -10.0, 10.0);
    }

    for (double& vLeak : VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }
#if 1
    for (Range& range : SpikeEncoderRange)
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

double Individual::EvaluateFitness(const FlappyBird& game) const
{
    double totalFitness = 0.0;
    for (const Player& player : Players)
    {
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        totalFitness += fitness;
    }

    double lowestFitness = std::numeric_limits<double>::max();
    for (const Player& player : Players)
    {
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        if (fitness < lowestFitness)
            lowestFitness = fitness;
    }

    constexpr double alpha = 0.9;
    return lowestFitness * alpha + (1.0 - alpha) * totalFitness / static_cast<double>(EVALUTIONS_PER_GENOME);
}

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng)
{
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    Genome childGenome = {};
    for (uint32_t i = 0; i < childGenome.Weights.size(); i++)
        childGenome.Weights[i] = (distribution(rng) < 0.5f) ? genomeA.Weights[i] : genomeB.Weights[i];

    for (uint32_t i = 0; i < childGenome.VLeaks.size(); i++)
        childGenome.VLeaks[i] = (distribution(rng) < 0.5f) ? genomeA.VLeaks[i] : genomeB.VLeaks[i];

    for (uint32_t i = 0; i < childGenome.SpikeEncoderRange.size(); i++)
        childGenome.SpikeEncoderRange[i] = (distribution(rng) < 0.5f) ? genomeA.SpikeEncoderRange[i] : genomeB.SpikeEncoderRange[i];

    return childGenome;
}

void ConstructNetwork(Individual& individual)
{
    NeuralNetwork& network = individual.BaseNetwork;
    const Genome& genome = individual.Genome;

    uint32_t weightCount = 0;
    for (uint32_t i = 0; i < INPUT_NEURONS; i++)
        for (uint32_t j = INPUT_NEURONS; j < INPUT_NEURONS + HIDDEN_NEURONS; j++)
            ConnectNeurons(network, j, i, genome.Weights[weightCount++]);

    for (uint32_t i = INPUT_NEURONS; i < INPUT_NEURONS + HIDDEN_NEURONS; i++)
        for (uint32_t j = INPUT_NEURONS + HIDDEN_NEURONS; j < TOTAL_NEURONS; j++)
            ConnectNeurons(network, j, i, genome.Weights[weightCount++]);

    for (uint32_t neuronIndex = 0; neuronIndex < TOTAL_NEURONS; neuronIndex++)
        network.Neurons[neuronIndex].V_leak = genome.VLeaks[neuronIndex];
}

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha)
{
    Assert((alpha >= 0.0) && (alpha <= 1.0));

    NoisyEvalConfig config;

    std::normal_distribution<double> weightNoise(0.0f, config.WeightNoiseSigma);
    std::normal_distribution<double> tauMemNoise(0.0, config.TauMemNoiseSigma);
    std::normal_distribution<double> tauSynNoise(0.0, config.TauSynNoiseSigma);
    std::normal_distribution<double> vThresholdNoise(0.0, config.VThresholdNoiseSigma);

    for (Neuron& neuron : network.Neurons)
    {
        for (double& weight : neuron.Weights)
            weight *= (1.0 + weightNoise(rng) * alpha);

        neuron.Tau_mem *= (1.0 + tauMemNoise(rng) * alpha);
        neuron.Tau_syn *= (1.0 + tauSynNoise(rng) * alpha);
        neuron.V_threshold *= (1.0 + vThresholdNoise(rng) * alpha);
    }
}