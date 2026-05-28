#pragma once

#include "ERP.h"
#include "Neuron.h"
#include "Game.h"

static constexpr uint32_t MAX_INDIVIDUALS = 512;
static constexpr uint32_t EVOLUTION_MU = 16;
static constexpr uint32_t MAX_EVALUTIONS_PER_GENOME = 16;
static constexpr double INITIAL_SIGMA = 5.0;
static constexpr uint32_t TOTAL_WEIGHTS = INPUT_NEURON_COUNT * HIDDEN_NEURON_COUNT + HIDDEN_NEURON_COUNT * OUTPUT_NEURON_COUNT;

static constexpr float CROSSOVER_CHANCE = 0.0f;
static constexpr float NEW_CONNECTION_CHANCE = 0.08f;
static constexpr float DELETE_CONNECTION_CHANCE = 0.02f;

static constexpr std::array<NeuronParams, HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT> NEURON_PARAMS = {
    {
        // Neuron 1
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 2
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 3
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 4
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 5
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 6
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 7
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
        // Neuron 8
        {
            .VDrive = 3.0,
            .TauMem = 0.02,
            .TauSyn = 0.02,
            .VLeak = 0.3,
            .VThreshold = 0.89
        },
    }
};

struct Connection
{
    double Weight = 0.0;
    int32_t InputNeuron = -1;
    int32_t OutputNeuron = -1;
};

struct Genome
{
    std::vector<Connection> Connections;

    Genome();

    void Print();
    void Mutate(std::mt19937& rng, double sigma);
};

struct Player
{
    NeuralNetwork Network = {};
    std::array<SpikeEncoder, INPUT_NEURON_COUNT> InputState = {};
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
    double TauMemNoiseSigma = 0.1;
    double TauSynNoiseSigma = 0.1;
    double VThresholdNoiseSigma = 0.02;
};

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha);