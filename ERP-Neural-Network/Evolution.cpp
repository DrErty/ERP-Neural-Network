#include "Evolution.h"

Genome::Genome()
{
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), 0);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), 1);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), 2);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(1), 3);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(1), 4);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(1), 5);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(2), 6);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(2), 7);

    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(3), 0);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(3), 1);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(3), 2);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(4), 3);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(4), 4);
    //Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(4), 5);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(5), 6);
    Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(5), 7);

    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(0));
    //Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(1));
    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(2));

    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(3));
    //Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(4));
    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(5));
}

static uint32_t CountInputConnections(const std::vector<Connection>& connections, int8_t neuron)
{
    uint32_t connectionCount = 0;

    for (auto& connection : connections)
    {
        if (connection.InputNeuron == neuron)
            connectionCount++;
    }

    return connectionCount;
}

static uint32_t CountOutputConnections(const std::vector<Connection>& connections, int8_t neuron)
{
    uint32_t connectionCount = 0;

    for (auto& connection : connections)
    {
        if (connection.OutputNeuron == neuron)
            connectionCount++;
    }

    return connectionCount;
}

static bool ValidConnection(const std::vector<Connection>& connections, int8_t inputNeuron, int8_t outputNeuron)
{
    uint32_t inputConnectionCount = 0;
    uint32_t outputConnectionCount = 0;

    for (auto& connection : connections)
    {
        if (connection.InputNeuron == inputNeuron && connection.OutputNeuron == outputNeuron)
        {
            return false;
        }

        if (connection.InputNeuron == inputNeuron)
            inputConnectionCount++;
        if (connection.OutputNeuron == outputNeuron)
            outputConnectionCount++;
    }

    /*
    for (auto& connection : connections)
    {
        if (GetNeuronIdxComplement(connection.InputNeuron) == inputNeuron && GetNeuronIdxComplement(connection.OutputNeuron) == outputNeuron)
        {
            return false;
        }

        if (GetNeuronIdxComplement(connection.InputNeuron) == inputNeuron)
            inputConnectionCount++;
        if (GetNeuronIdxComplement(connection.OutputNeuron) == outputNeuron)
            outputConnectionCount++;
    }
    */

    if (inputConnectionCount >= MAX_INPUTS)
        return false;

    if (outputConnectionCount >= MAX_OUTPUTS)
        return false;

    return true;
}

void Genome::Print()
{
    std::cout << "Connections, count: " << Connections.size() << ": ";

    for (auto& connection : Connections)
    {
        std::cout << "In: " << connection.InputNeuron << " Out: " << connection.OutputNeuron << " Weight:" << connection.Weight << '\n';
    }
    /*
    std::cout << "VLeaks: ";
    for (double vLeak : VLeaks)
    {
        std::cout << vLeak << " ";
    }
    std::cout << '\n';
    */
}

void Genome::Mutate(std::mt19937& rng, double sigma)
{
    std::uniform_real_distribution<double> uniformDistribution(0.0f, 1.0f);
    std::normal_distribution<double> normalDistribution(0.0, sigma);
    std::normal_distribution<double> vLeakDistribution(0.0, sigma * 0.1);

    for (auto& connection : Connections)
    {
        connection.Weight += normalDistribution(rng);
        connection.Weight = std::clamp(connection.Weight, -3.5, 3.5);
    }

    /*
    for (double& vLeak : VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }
    */

    /*
    while ((uniformDistribution(rng) <= DELETE_CONNECTION_CHANCE) && (Connections.size() > 0))
    {
        std::uniform_int_distribution<uint32_t> distribution(0, Connections.size() - 1);

        Connections.erase(Connections.begin() + distribution(rng));
    }

    while (uniformDistribution(rng) <= NEW_CONNECTION_CHANCE)
    {
        std::uniform_int_distribution<uint32_t> inputDistribution(INPUT_NEURONS, TOTAL_NEURONS - 1);
        std::uniform_int_distribution<uint32_t> outputDistribution(0, INPUT_NEURONS + HIDDEN_NEURONS - 1);
        while (true)
        {
            uint32_t inputNeuron = inputDistribution(rng);
            uint32_t outputNeuron = outputDistribution(rng);
            if (inputNeuron != outputNeuron)
            {
                if (ValidConnection(Connections, inputNeuron, outputNeuron) && ValidConnection(Connections, GetNeuronComplement(inputNeuron), GetNeuronComplement(outputNeuron)))
                {
                    Connections.emplace_back(0.0, inputNeuron, outputNeuron);
                }
                break;
            }
        }
    }
    */
}

double Individual::EvaluateFitness(const Game& game) const
{
    double totalGameFitness = 0.0;
    for (uint32_t playerIndex = 0; playerIndex < EvalutationsPerGenome; playerIndex++)
    {
        const Player& player = Players2[playerIndex];
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        totalGameFitness += fitness;
    }

    double lowestFitness = std::numeric_limits<double>::max();
    for (uint32_t playerIndex = 0; playerIndex < EvalutationsPerGenome; playerIndex++)
    {
        const Player& player = Players2[playerIndex];
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        if (fitness < lowestFitness)
            lowestFitness = fitness;
    }

    constexpr double alpha = 0.75;
    const double gameFitness = lowestFitness * alpha + (1.0 - alpha) * totalGameFitness / static_cast<double>(EvalutationsPerGenome);

    double totalFitness = gameFitness;

    return totalFitness;
}

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng)
{
    Assert(false);

    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    Genome childGenome = {};
    //for (uint32_t i = 0; i < childGenome.Weights.size(); i++)
    //    childGenome.Weights[i] = (distribution(rng) < 0.5f) ? genomeA.Weights[i] : genomeB.Weights[i];

    //for (uint32_t i = 0; i < childGenome.VLeaks.size(); i++)
    //    childGenome.VLeaks[i] = (distribution(rng) < 0.5f) ? genomeA.VLeaks[i] : genomeB.VLeaks[i];

    return childGenome;
}

void ConstructNetwork(Individual& individual)
{
    NeuralNetwork& network = individual.BaseNetwork;
    const Genome& genome = individual.Genome;

    for (uint32_t inputIdx = 0; inputIdx < INPUT_NEURON_COUNT; inputIdx++)
        network.Neurons[GetNeuronIdxFromInputIdx(inputIdx)].Inactive = true;

    for (auto& connection : genome.Connections)
    {
        ConnectNeurons(network, connection.InputNeuron, connection.OutputNeuron, connection.Weight);
        //ConnectNeurons(network, GetNeuronIdxComplement(connection.InputNeuron), GetNeuronIdxComplement(connection.OutputNeuron), connection.Weight);
    }

    for (uint32_t neuronIdx = INPUT_NEURON_COUNT; neuronIdx < TOTAL_NEURON_COUNT; neuronIdx++)
    {
        network.Neurons[neuronIdx].Params = NEURON_PARAMS[neuronIdx - INPUT_NEURON_COUNT];
    }
}

void VaryNetwork(NeuralNetwork& network, std::mt19937& rng, double alpha)
{
    Assert((alpha >= 0.0) && (alpha <= 1.0));

    NoisyEvalConfig config;

    std::normal_distribution<double> weightNoise(0.0, config.WeightNoiseSigma);
    std::normal_distribution<double> tauMemNoise(0.0, config.TauMemNoiseSigma);
    std::normal_distribution<double> tauSynNoise(0.0, config.TauSynNoiseSigma);
    std::normal_distribution<double> vThresholdNoise(0.0, config.VThresholdNoiseSigma);

    for (Neuron& neuron : network.Neurons)
    {
        for (double& weight : neuron.Weights)
            weight *= (1.0 + std::clamp(tauMemNoise(rng) * alpha, -config.WeightNoiseSigma, config.WeightNoiseSigma));

        neuron.Params.TauMem *= (1.0 + std::clamp(tauMemNoise(rng) * alpha, -config.TauMemNoiseSigma, config.TauMemNoiseSigma));
        neuron.Params.TauSyn *= (1.0 + std::clamp(tauSynNoise(rng) * alpha, -config.TauSynNoiseSigma, config.TauSynNoiseSigma));
        neuron.Params.VThreshold *= (1.0 + std::clamp(vThresholdNoise(rng) * alpha, -config.VThresholdNoiseSigma, config.VThresholdNoiseSigma));
    }
}