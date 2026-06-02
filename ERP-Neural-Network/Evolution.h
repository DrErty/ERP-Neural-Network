#pragma once

#include "ERP.h"
#include "Neuron.h"

#include "CartPole.h"

static constexpr uint32_t MAX_INDIVIDUALS = 1024 * 4;
static constexpr uint32_t EVOLUTION_MU = 1024;
static constexpr uint32_t MAX_EVALUTIONS_PER_GENOME = 4;
static constexpr double INITIAL_SIGMA = 10.0;
static constexpr double INITIAL_NEW_WEIGHT_SIGMA = 1.0;

static constexpr float CROSSOVER_CHANCE = 0.0f;
static constexpr float NEW_CONNECTION_CHANCE = 0.8f;
static constexpr float DELETE_CONNECTION_CHANCE = 0.5f;

static constexpr bool MUTABLE_TOPOLOGY = false;

struct NoisyEvalConfig
{
    double WeightNoiseSigma = 0.3;
    double TauMemNoiseSigma = 0.1;
    double TauSynNoiseSigma = 0.2;
    double VThresholdNoiseSigma = 0.02;
};

static constexpr std::array<NeuronParams, HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT> NEURON_PARAMS = {
    {
        /*
        // Neuron 1
        {
            .VDrive = 3.0,
            .TauMem = 0.0330666,
            .TauSyn = 0.05,
            .VLeak = 0.482946559996318,
            .VThreshold = 0.89
        },
        // Neuron 2
        {
            .VDrive = 3.0,
            .TauMem = 0.0232725,
            .TauSyn = 0.05,
            .VLeak = 0.6280052380709833,
            .VThreshold = 0.89
        },
        // Neuron 3
        {
            .VDrive = 3.0,
            .TauMem = 0.02471013,
            .TauSyn = 0.05,
            .VLeak = 0.593853894281843,
            .VThreshold = 0.89
        },
        // Neuron 4
        {
            .VDrive = 3.0,
            .TauMem = 0.01524596,
            .TauSyn = 0.05,
            .VLeak = 0.5725515709282208,
            .VThreshold = 0.89
        },
        // Neuron 5
        {
            .VDrive = 3.0,
            .TauMem = 0.0115239,
            .TauSyn = 0.05,
            .VLeak = 0.5532780402749436,
            .VThreshold = 0.89
        },
        // Neuron 6
        {
            .VDrive = 3.0,
            .TauMem = 0.01565709,
            .TauSyn = 0.05,
            .VLeak = 0.5275799994039073,
            .VThreshold = 0.89
        },
        // Neuron 7
        {
            .VDrive = 3.0,
            .TauMem = 0.04,
            .TauSyn = 0.05,
            .VLeak = 0.482946559996318,
            .VThreshold = 0.89
        },
        // Neuron 8
        {
            .VDrive = 3.0,
            .TauMem = 0.0407612,
            .TauSyn = 0.08151,
            .VLeak = 0.6375,
            .VThreshold = 1.244
        },
        */
        {
            .VDrive = 3.0,
            .TauMem = 0.5,
            .TauSyn = 0.08,
            .VLeak = 0.0,
            .VThreshold = 1.244
        }
    }
};

struct Connection
{
    Connection(double weight, int8_t inputNeuron, int8_t outputNeuron, bool deletable)
        : Weight(weight), Sigma(INITIAL_SIGMA), InputNeuron(inputNeuron), OutputNeuron(outputNeuron), Deletable(deletable)
    {

    }
    Connection() = delete;

    double Weight;
    double Sigma;
    int8_t InputNeuron;
    int8_t OutputNeuron;
    bool Deletable = true;
};

struct Genome
{
    std::vector<Connection> Connections;

    std::array<NeuronParams, HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT> Params = NEURON_PARAMS;

    double Sigma = INITIAL_SIGMA;

    Genome();

    size_t FindConnection(NeuronIdx inputNeuron, NeuronIdx outputNeuron) const;

    void Print();
    void Mutate(std::mt19937& rng);
};

struct Player
{
    Player();

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

    double EvaluateFitness(const CartPole& game) const;
};

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng);

void ConstructNetwork(Individual& individual);

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha);