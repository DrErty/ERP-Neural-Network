#include "Evolution.h"

Genome::Genome()
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

static uint32_t GetNeuronComplement(uint32_t neuron)
{
    if (neuron < INPUT_NEURONS)
        return INPUT_NEURONS - neuron - 1;
    else if ((neuron >= INPUT_NEURONS) && (neuron < (INPUT_NEURONS + HIDDEN_NEURONS)))
        return (HIDDEN_NEURONS - (neuron - INPUT_NEURONS) - 1) + INPUT_NEURONS;
    else
        return (OUTPUT_NEURONS - (neuron - INPUT_NEURONS - HIDDEN_NEURONS) - 1) + INPUT_NEURONS + HIDDEN_NEURONS;
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

    for (auto& connection : connections)
    {
        if (GetNeuronComplement(connection.InputNeuron) == inputNeuron && GetNeuronComplement(connection.OutputNeuron) == outputNeuron)
        {
            return false;
        }

        if (GetNeuronComplement(connection.InputNeuron) == inputNeuron)
            inputConnectionCount++;
        if (GetNeuronComplement(connection.OutputNeuron) == outputNeuron)
            outputConnectionCount++;
    }

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
    std::cout << "VLeaks: ";
    for (double vLeak : VLeaks)
    {
        std::cout << vLeak << " ";
    }
    std::cout << '\n';
}

void Genome::Mutate(std::mt19937& rng, double sigma)
{
    std::uniform_real_distribution<double> uniformDistribution(0.0f, 1.0f);
    std::normal_distribution<double> normalDistribution(0.0, sigma);
    std::normal_distribution<double> vLeakDistribution(0.0, sigma * 0.1);

    for (auto& connection : Connections)
    {
        connection.Weight += normalDistribution(rng);
        connection.Weight = std::clamp(connection.Weight, -20.0, 20.0);
    }

    for (double& vLeak : VLeaks)
    {
        vLeak += vLeakDistribution(rng);
        vLeak = std::clamp(vLeak, 0.0, V_DD);
    }

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
    for (const Player& player : Players)
    {
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        totalGameFitness += fitness;
    }

    double lowestFitness = std::numeric_limits<double>::max();
    for (const Player& player : Players)
    {
        const double fitness = game.PlayerFitness(player.PlayerIndex);
        if (fitness < lowestFitness)
            lowestFitness = fitness;
    }

    constexpr double alpha = 0.75;
    const double gameFitness = lowestFitness * alpha + (1.0 - alpha) * totalGameFitness / static_cast<double>(EVALUTIONS_PER_GENOME);

    double totalFitness = gameFitness;
    for (uint32_t neuron = INPUT_NEURONS + HIDDEN_NEURONS; neuron < TOTAL_NEURONS; neuron++)
    {
        //totalFitness -= CountInputConnections(Genome.Connections, neuron) > 0 ? 0.0 : 100.0;
    }
    for (uint32_t neuron = 0; neuron < INPUT_NEURONS; neuron++)
    {
        //totalFitness -= CountOutputConnections(Genome.Connections, neuron) > 0 ? 0.0 : 100.0;
    }

    return totalFitness;
}

Genome CrossoverGenome(const Genome& genomeA, const Genome& genomeB, std::mt19937& rng)
{
    Assert(false);

    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    Genome childGenome = {};
    //for (uint32_t i = 0; i < childGenome.Weights.size(); i++)
    //    childGenome.Weights[i] = (distribution(rng) < 0.5f) ? genomeA.Weights[i] : genomeB.Weights[i];

    for (uint32_t i = 0; i < childGenome.VLeaks.size(); i++)
        childGenome.VLeaks[i] = (distribution(rng) < 0.5f) ? genomeA.VLeaks[i] : genomeB.VLeaks[i];

    return childGenome;
}

void ConstructNetwork(Individual& individual)
{
    NeuralNetwork& network = individual.BaseNetwork;
    const Genome& genome = individual.Genome;

    for (uint32_t i = 0; i < INPUT_NEURONS; i++)
        network.Neurons[i].Inactive = true;

    for (auto& connection : genome.Connections)
    {
        //std::printf("In: %i, Out: %i\n", connection.InputNeuron, connection.OutputNeuron);
        //std::printf("Complement In: %i, Out: %i\n", GetNeuronComplement(connection.InputNeuron), GetNeuronComplement(connection.OutputNeuron));
        ConnectNeurons(network, connection.InputNeuron, connection.OutputNeuron, connection.Weight);
        ConnectNeurons(network, GetNeuronComplement(connection.InputNeuron), GetNeuronComplement(connection.OutputNeuron), connection.Weight);
    }

    for (uint32_t neuronIndex = INPUT_NEURONS; neuronIndex < INPUT_NEURONS + HIDDEN_NEURONS / 2; neuronIndex++)
    {
        network.Neurons[neuronIndex].V_leak = genome.VLeaks[neuronIndex - INPUT_NEURONS];
        network.Neurons[GetNeuronComplement(neuronIndex)].V_leak = genome.VLeaks[neuronIndex - INPUT_NEURONS];
    }

    for (uint32_t neuronIndex = INPUT_NEURONS + HIDDEN_NEURONS; neuronIndex < INPUT_NEURONS + HIDDEN_NEURONS + OUTPUT_NEURONS / 2; neuronIndex++)
    {
        network.Neurons[neuronIndex].V_leak = genome.VLeaks[neuronIndex - INPUT_NEURONS - HIDDEN_NEURONS + HIDDEN_NEURONS / 2];
        network.Neurons[GetNeuronComplement(neuronIndex)].V_leak = genome.VLeaks[neuronIndex - INPUT_NEURONS - HIDDEN_NEURONS + HIDDEN_NEURONS / 2];
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

        neuron.Tau_mem *= (1.0 + std::clamp(tauMemNoise(rng) * alpha, -config.TauMemNoiseSigma, config.TauMemNoiseSigma));
        neuron.Tau_syn *= (1.0 + std::clamp(tauSynNoise(rng) * alpha, -config.TauSynNoiseSigma, config.TauSynNoiseSigma));
        neuron.V_threshold *= (1.0 + std::clamp(vThresholdNoise(rng) * alpha, -config.VThresholdNoiseSigma, config.VThresholdNoiseSigma));
    }
}