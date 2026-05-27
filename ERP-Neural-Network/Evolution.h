#pragma once

#include "ERP.h"
#include "Neuron.h"
#include "Game.h"

static constexpr uint32_t MAX_INDIVIDUALS = 1024;
static constexpr uint32_t EVOLUTION_MU = 16;
static constexpr uint32_t MAX_EVALUTIONS_PER_GENOME = 4;
static constexpr double INITIAL_SIGMA = 5.0;
static constexpr uint32_t TOTAL_WEIGHTS = INPUT_NEURONS * HIDDEN_NEURONS + HIDDEN_NEURONS * OUTPUT_NEURONS;

static constexpr float CROSSOVER_CHANCE = 0.0f;
static constexpr float NEW_CONNECTION_CHANCE = 0.08f;
static constexpr float DELETE_CONNECTION_CHANCE = 0.02f;

struct Connection
{
    double Weight = 0.0;
    int32_t InputNeuron = -1;
    int32_t OutputNeuron = -1;
};

struct Genome
{
    std::vector<Connection> Connections;
    std::array<double, OUTPUT_NEURONS / 2 + HIDDEN_NEURONS / 2> VLeaks = {};

    Genome();

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
    std::array<Player, MAX_EVALUTIONS_PER_GENOME> Players2 = {};
    NeuralNetwork BaseNetwork = {};
    uint32_t EvalutationsPerGenome = 1;
    bool Alive = true;

    double EvaluateFitness(const Game& game) const;
};

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng);

void ConstructNetwork(Individual& individual);

struct NoisyEvalConfig
{
    double WeightNoiseSigma = 0.2;
    double TauMemNoiseSigma = 0.05;
    double TauSynNoiseSigma = 0.2;
    double VThresholdNoiseSigma = 0.02;
};

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha);