#pragma once

#include "ERP.h"
#include "Neuron.h"

#include "CartPole.h"

struct NoisyEvalConfig
{
    Scalar WeightNoiseSigma = Scalar(0.3);
    Scalar TauMemNoiseSigma = Scalar(0.1);
    Scalar TauSynNoiseSigma = Scalar(0.2);
    Scalar VThresholdNoiseSigma = Scalar(0.02);
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
            .VDrive = Scalar(3.0),
            .TauMem = Scalar(0.5),
            .TauSyn = Scalar(0.08),
            .VLeak = Scalar(0.0),
            .VThreshold = Scalar(1.244)
        }
    }
};

struct Connection
{
    Connection(Scalar weight, int8_t inputNeuron, int8_t outputNeuron, bool deletable)
        : Weight(weight), Sigma(INITIAL_SIGMA), InputNeuron(inputNeuron), OutputNeuron(outputNeuron), Deletable(deletable)
    {

    }
    Connection() = delete;

    Scalar Weight;
    Scalar Sigma;
    int8_t InputNeuron;
    int8_t OutputNeuron;
    bool Deletable = true;
};

struct Genome
{
    std::vector<Connection> Connections;

    std::array<NeuronParams, HIDDEN_NEURON_COUNT + OUTPUT_NEURON_COUNT> Params = NEURON_PARAMS;

    Scalar Sigma = INITIAL_SIGMA;

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

    Scalar EvaluateFitness(const CartPole& game) const;
};

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng);

void ConstructNetwork(Individual& individual);

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, Scalar alpha);