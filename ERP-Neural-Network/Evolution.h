#pragma once

#include "ERP.h"
#include "Neuron.h"
#include "Game.h"

static constexpr uint32_t MAX_INDIVIDUALS = 256;
static constexpr uint32_t EVOLUTION_MU = 8;
static constexpr uint32_t EVALUTIONS_PER_GENOME = 32;
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
    std::array<double, TOTAL_NEURONS> VLeaks = {};

    Genome()
    {
        Connections.emplace_back(0.0, INPUT_NEURONS + 0, 0);
        Connections.emplace_back(0.0, INPUT_NEURONS + 0, 1);
        Connections.emplace_back(0.0, INPUT_NEURONS + 0, 2);
        Connections.emplace_back(0.0, INPUT_NEURONS + 1, 3);
        Connections.emplace_back(0.0, INPUT_NEURONS + 1, 4);
        Connections.emplace_back(0.0, INPUT_NEURONS + 1, 5);
        Connections.emplace_back(0.0, INPUT_NEURONS + 2, 6);
        Connections.emplace_back(0.0, INPUT_NEURONS + 2, 7);

        Connections.emplace_back(0.0, INPUT_NEURONS + HIDDEN_NEURONS + 0, INPUT_NEURONS + 0);
        Connections.emplace_back(0.0, INPUT_NEURONS + HIDDEN_NEURONS + 0, INPUT_NEURONS + 1);
        Connections.emplace_back(0.0, INPUT_NEURONS + HIDDEN_NEURONS + 0, INPUT_NEURONS + 2);
    }

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

    double EvaluateFitness(const Game& game) const;
};

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng);

void ConstructNetwork(Individual& individual);

struct NoisyEvalConfig
{
    double WeightNoiseSigma = 0.1;
    double TauMemNoiseSigma = 0.05;
    double TauSynNoiseSigma = 0.1;
    double VThresholdNoiseSigma = 0.02;
};

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha);