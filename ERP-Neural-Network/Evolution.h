#pragma once

#include "ERP.h"
#include "Neuron.h"
#include "FlappyBird.h"

static constexpr uint32_t MAX_INDIVIDUALS = 200;
static constexpr uint32_t EVALUTIONS_PER_GENOME = 5;
static constexpr double INITIAL_SIGMA = 5.0;
static constexpr uint32_t TOTAL_WEIGHTS = INPUT_NEURONS * HIDDEN_NEURONS + HIDDEN_NEURONS * OUTPUT_NEURONS;

static constexpr float CROSSOVER_CHANCE = 0.75f;

struct Range
{
    double Min = 0.0;
    double Max = 1.0;
};

struct Genome
{
    std::array<double, TOTAL_WEIGHTS> Weights = {};
    std::array<double, TOTAL_NEURONS> VLeaks = {};
    std::array<Range, INPUT_NEURONS> SpikeEncoderRange = {};

    void Print();
    void Mutate(std::mt19937& rng, double sigma);
};

struct Player
{
    NeuralNetwork Network = {};
    std::array<SpikeEncoder, INPUT_NEURONS> InputState = {};
    uint32_t PlayerIndex = 0;
};

struct Individual
{
    Genome Genome = {};
    std::array<Player, EVALUTIONS_PER_GENOME> Players = {};
    NeuralNetwork BaseNetwork = {};
    bool Alive = true;

    double EvaluateFitness(const FlappyBird& game) const;
};

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng);

void ConstructNetwork(Individual& individual);

struct NoisyEvalConfig
{
    float WeightNoiseSigma = 0.1f;
    float TauMemNoiseSigma = 0.05f;
    float TauSynNoiseSigma = 0.1f;
    float VThresholdNoiseSigma = 0.02f;
};

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha);