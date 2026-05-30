#include "Evolution.h"

Genome::Genome()
{
    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), 0, false);
    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), 1, false);

    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), 6, false);
    Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), 7, false);
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
    std::cout << "Connections, count: " << Connections.size() << ": \n";

    for (auto& connection : Connections)
    {
        std::cout << "In: " << connection.InputNeuron << " Out: " << connection.OutputNeuron << " Weight:" << connection.Weight << " Sigma:" << connection.Sigma << '\n';
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

void Genome::Mutate(std::mt19937& rng)
{
    std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
    std::normal_distribution<double> n01(0.0, 1.0);

    const double n = static_cast<double>(std::max<size_t>(1, Connections.size()));
    const double tauGlobal = 1.0 / std::sqrt(2.0 * n);
    const double tauLocal = 1.0 / std::sqrt(2.0 * std::sqrt(n));

    const double globalFactor = std::exp(tauGlobal * n01(rng));

    for (auto& connection : Connections)
    {
        connection.Sigma *= globalFactor * std::exp(tauLocal * n01(rng));
        connection.Sigma = std::clamp(connection.Sigma, 0.0, INITIAL_SIGMA);

        std::normal_distribution<double> weightStep(0.0, connection.Sigma);
        connection.Weight += weightStep(rng);
        connection.Weight = std::clamp(connection.Weight, -MAX_WEIGHT, MAX_WEIGHT);
    }

    /*
    std::normal_distribution<double> n01(0.0, 1.0);
    const double tau = 1.0 / std::sqrt(2.0 * std::max<size_t>(1, Connections.size()));
    Sigma *= std::exp(tau * n01(rng));
    Sigma = std::clamp(Sigma, 1e-3, INITIAL_SIGMA);

    std::normal_distribution<double> weightStep(0.0, Sigma * 0.1);
    for (auto& connection : Connections) {
        connection.Weight += weightStep(rng);
        connection.Weight = std::clamp(connection.Weight, -MAX_WEIGHT, MAX_WEIGHT);
    }

    std::uniform_real_distribution<double> uniformDistribution(0.0f, 1.0f);
    */

    /*
    for (double& vLeak : VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }
    */

    while ((uniformDistribution(rng) <= DELETE_CONNECTION_CHANCE) && (Connections.size() > 0))
    {
        std::uniform_int_distribution<uint32_t> distribution(0, Connections.size() - 1);
        uint32_t deleteIndex = distribution(rng);
        if (Connections[deleteIndex].Deletable)
        {
            Connections.erase(Connections.begin() + deleteIndex);
        }
    }

    while (uniformDistribution(rng) <= NEW_CONNECTION_CHANCE)
    {
        std::uniform_int_distribution<uint32_t> inputDistribution(INPUT_NEURON_COUNT, TOTAL_NEURON_COUNT - 1);
        std::uniform_int_distribution<uint32_t> outputDistribution(0, INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT - 1);
        while (true)
        {
            uint32_t inputNeuron = inputDistribution(rng);
            uint32_t outputNeuron = outputDistribution(rng);
            if (inputNeuron != outputNeuron)
            {
                if (ValidConnection(Connections, inputNeuron, outputNeuron))
                {
                    Connections.emplace_back(0.0, inputNeuron, outputNeuron, true);
                }
                break;
            }
        }
    }
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
    const double gameFitness = alpha * lowestFitness + (1.0 - alpha) * totalGameFitness / static_cast<double>(EvalutationsPerGenome);

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
        network.InactiveNeurons[GetNeuronIdxFromInputIdx(inputIdx)] = true;

    for (auto& connection : genome.Connections)
    {
        ConnectNeurons(network, connection.InputNeuron, connection.OutputNeuron, connection.Weight);
    }

    for (uint32_t neuronIdx = INPUT_NEURON_COUNT; neuronIdx < TOTAL_NEURON_COUNT; neuronIdx++)
    {
        network.SetParams(neuronIdx, NEURON_PARAMS[neuronIdx - INPUT_NEURON_COUNT]);
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

    for (auto& neuronWeights : network.Weights)
        for (auto& weight : neuronWeights)
            weight *= (1.0 + std::clamp(weightNoise(rng) * alpha, -config.WeightNoiseSigma, config.WeightNoiseSigma));

    for (uint32_t neuronIdx = 0; neuronIdx < TOTAL_NEURON_COUNT; neuronIdx++)
    {
        NeuronParams params = network.GetParams(neuronIdx);
        params.TauMem *= (1.0 + std::clamp(tauMemNoise(rng) * alpha, -config.TauMemNoiseSigma, config.TauMemNoiseSigma));
        params.TauSyn *= (1.0 + std::clamp(tauSynNoise(rng) * alpha, -config.TauSynNoiseSigma, config.TauSynNoiseSigma));
        params.VThreshold *= (1.0 + std::clamp(vThresholdNoise(rng) * alpha, -config.VThresholdNoiseSigma, config.VThresholdNoiseSigma));
        network.SetParams(neuronIdx, params);
    }
}