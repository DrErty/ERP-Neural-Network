#include "Evolution.h"

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

Genome::Genome()
{
    if (INPUT_NEURON_COUNT == 8)
    {
        if (true)
        {
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromInputIdx(0), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromInputIdx(1), false);

            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(0), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), false);

            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), GetNeuronIdxFromInputIdx(2), true);
            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), GetNeuronIdxFromInputIdx(3), true);

            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), GetNeuronIdxFromInputIdx(4), true);
            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), GetNeuronIdxFromInputIdx(5), true);

            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromInputIdx(6), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromInputIdx(7), false);
        }
        else
        {
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(0), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(1), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 2), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(2), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 3), false);

            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), GetNeuronIdxFromInputIdx(INPUT_NEURON_COUNT - 1), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(0), GetNeuronIdxFromInputIdx(INPUT_NEURON_COUNT - 2), false);

            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), GetNeuronIdxFromInputIdx(1), false);
            Connections.emplace_back(0.0, GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), GetNeuronIdxFromInputIdx(0), false);
        }
    }
    else if (INPUT_NEURON_COUNT == 4)
    {
        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromInputIdx(0), false);
        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromInputIdx(1), false);

        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromHiddenIdx(0), false);
        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(1), GetNeuronIdxFromHiddenIdx(HIDDEN_NEURON_COUNT - 1), false);

        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromInputIdx(2), false);
        Connections.emplace_back(0.0, GetNeuronIdxFromOutputIdx(0), GetNeuronIdxFromInputIdx(3), false);
    }
}

size_t Genome::FindConnection(NeuronIdx inputNeuron, NeuronIdx outputNeuron) const
{
    for (size_t connectionIdx = 0; connectionIdx < Connections.size(); connectionIdx++)
    {
        const auto& connection = Connections[connectionIdx];
        if (connection.InputNeuron == inputNeuron and connection.OutputNeuron == outputNeuron)
        {
            return connectionIdx;
        }
    }
    // TODO: Handle error
    Assert(false);
    return 0;
}

void Genome::Print()
{
    std::cout << "Connections, count: " << Connections.size() << ": \n";

    for (auto& connection : Connections)
    {
        std::cout << "In: " << static_cast<uint32_t>(connection.InputNeuron) 
            << " Out: " << static_cast<uint32_t>(connection.OutputNeuron)
            << " Weight:" << connection.Weight
            << " Sigma:" << connection.Sigma << '\n';
    }

    for (auto& param : Params)
    {
        param.Print();
    }
}

void Genome::Mutate(std::mt19937& rng)
{
    std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
    std::normal_distribution<double> n01(0.0, 1.0);

    {
        const double tau = 1.0 / std::sqrt(2.0 * std::max<size_t>(1, Connections.size()));
        Sigma *= std::exp(tau * n01(rng));
        Sigma = std::clamp(Sigma, 1e-3, INITIAL_SIGMA);

        std::normal_distribution<double> step(0.0, Sigma * 0.1);
        for (auto& param : Params) {
            param.TauMem += step(rng);
            param.TauMem = std::clamp(param.TauMem, 0.01, 1.0);

            param.TauSyn += step(rng);
            param.TauSyn = std::clamp(param.TauSyn, 0.01, 1.0);
        }
    }

    /*
    for (double& vLeak : VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }
    */

    if (MUTABLE_TOPOLOGY)
    {
        while ((uniformDistribution(rng) <= DELETE_CONNECTION_CHANCE) && (Connections.size() > 0))
        {
            std::uniform_int_distribution<uint32_t> distribution(0, Connections.size() - 1);
            const uint32_t deleteIdx = distribution(rng);
            if (Connections[deleteIdx].Deletable)
            {
                const NeuronIdx inputNeuron = Connections[deleteIdx].InputNeuron;
                const NeuronIdx outputNeuron = Connections[deleteIdx].OutputNeuron;
                Connections.erase(Connections.begin() + deleteIdx);
                const uint32_t deleteCompIdx = FindConnection(GetNeuronIdxComplement(inputNeuron), GetNeuronIdxComplement(outputNeuron));
                Assert(Connections[deleteCompIdx].Deletable);
                Connections.erase(Connections.begin() + deleteCompIdx);
            }
            Assert(Connections.size() % 2 == 0);
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
                    if (ValidConnection(Connections, inputNeuron, outputNeuron) and ValidConnection(Connections, GetNeuronIdxComplement(inputNeuron), GetNeuronIdxComplement(outputNeuron)))
                    {
                        Connections.emplace_back(0.0, inputNeuron, outputNeuron, true);
                        Connections.emplace_back(0.0, GetNeuronIdxComplement(inputNeuron), GetNeuronIdxComplement(outputNeuron), true);

                        Assert(Connections.size() % 2 == 0);
                    }
                    break;
                }
            }
        }
    }

    const double n = static_cast<double>(std::max<size_t>(1, Connections.size()));
    const double tauGlobal = 1.0 / std::sqrt(2.0 * n);
    const double tauLocal = 1.0 / std::sqrt(2.0 * std::sqrt(n));

    const double globalFactor = std::exp(tauGlobal * n01(rng));

    for (auto& connection : Connections)
    {
        connection.Sigma *= globalFactor * std::exp(tauLocal * n01(rng));
        connection.Sigma = std::clamp(connection.Sigma, 0.0, INITIAL_NEW_WEIGHT_SIGMA);

        std::normal_distribution<double> weightStep(0.0, connection.Sigma);
        connection.Weight += weightStep(rng);
        connection.Weight = std::clamp(connection.Weight, -MAX_WEIGHT, MAX_WEIGHT);
    }
}

double Individual::EvaluateFitness(const CartPole& game) const
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
        network.SetParams(neuronIdx, genome.Params[neuronIdx - INPUT_NEURON_COUNT]);
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