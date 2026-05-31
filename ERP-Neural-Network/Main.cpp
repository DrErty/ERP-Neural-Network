#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "serialib.h"

#include "Drawer.h"
#include "CartPole.h"
#include "Neuron.h"
#include "Evolution.h"

#include "SimulationUI.h"

static constexpr float M_PI = 3.14159265f;
static constexpr float NEURON_SIZE = 80.0f;
static constexpr float METER_WIDTH = NEURON_SIZE;
static constexpr float METER_HEIGHT = 16.0f;
static constexpr float METER_GAP = 8.0f;

static constexpr double MAX_GAME_TIME = 30.0;

static constexpr uint32_t STRICT_MODE_START = 1;
static constexpr uint32_t ALPHA_START = 50;
static constexpr uint32_t ALPHA_DURATION = 10;
static constexpr uint32_t MULTI_EVAL_START = 1;

struct Renderer
{
    SDL_Renderer* Renderer;
    SDL_Window* Window;
};

static void StartSDLTTF()
{
    if (!TTF_Init())
    {
        SDL_Log("TTF_Init: %s", SDL_GetError());
        SDL_Quit();
    }

    Drawer::g_FontSmall = TTF_OpenFont("font.ttf", 12.0f);
    Drawer::g_FontMedium = TTF_OpenFont("font.ttf", 16.0f);
    if (!Drawer::g_FontSmall || !Drawer::g_FontMedium)
    {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
    }
}

static void StopSDLTTF()
{
    if (Drawer::g_FontSmall)
        TTF_CloseFont(Drawer::g_FontSmall);

    if (Drawer::g_FontMedium)
        TTF_CloseFont(Drawer::g_FontMedium);

    TTF_Quit();
}

// TODO: RAII clean up
static Renderer StartSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("Evolutionary Training", Drawer::DEFAULT_WINDOW_WIDTH, Drawer::DEFAULT_WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    StartSDLTTF();

    return { renderer, window };
}

static void StopSDL(Renderer renderer)
{
    StopSDLTTF();

    SDL_DestroyRenderer(renderer.Renderer);
    SDL_DestroyWindow(renderer.Window);
    SDL_Quit();
}

static void SerialReadToBuffer(serialib& serial, std::string& buffer)
{
    while (serial.available() > 0)
    {
        char readChar;
        serial.readChar(&readChar, 0);
        
        buffer += readChar;
    }
}

static double CalculateDeltaTime(Uint64 start, Uint64 end)
{
    Assert(end > start);
    return static_cast<double>(end - start) / static_cast<double>(SDL_GetPerformanceFrequency());
}

struct Frame
{
    Uint64 Start;
};

static Frame FrameStart(const Renderer& renderer)
{
    return { SDL_GetPerformanceCounter() };
}

static double FrameEnd(const Renderer& renderer, const Frame& frame, bool speedUp)
{
    SDL_RenderPresent(renderer.Renderer);

    {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        const double elapsedTime = CalculateDeltaTime(frame.Start, currentTime);
        if ((elapsedTime < SIM_DT) && (!speedUp))
            SDL_Delay(static_cast<Uint32>((SIM_DT - elapsedTime) * 1000.0));
    }

    Uint64 currentTimeAfterDelay = SDL_GetPerformanceCounter();
    const double elapsedTimeAfterDelay = CalculateDeltaTime(frame.Start, currentTimeAfterDelay);

    return elapsedTimeAfterDelay;
}

struct GameState
{
    bool Quit = false;
    bool SpeedUp = false;
    bool Render = true;
    bool Skip = false;
    bool Disable = false;
    bool TrackingCamera = false;
    uint32_t Generation = 0;
};

static void HandleGameInputs(GameState& gameState)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            gameState.Quit = true;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
                gameState.Quit = true;
            if (event.key.key == SDLK_SPACE)
                gameState.SpeedUp = !gameState.SpeedUp;
            if (event.key.key == SDLK_R)
                gameState.Render = !gameState.Render;
            if (event.key.key == SDLK_P)
                gameState.Skip = true;
            if (event.key.key == SDLK_L)
                gameState.Disable = !gameState.Disable;
            if (event.key.key == SDLK_T)
                gameState.TrackingCamera = !gameState.TrackingCamera;
        }
    }
}

static void UpdateInputs(const Game& game, Player& player)
{
    for (uint32_t inputIdx = 0; inputIdx < INPUT_NEURON_COUNT; inputIdx++)
    {
        float input = game.GetInput(player.PlayerIndex, inputIdx);
        player.InputState[inputIdx].SetValue(input, 0.0f, 1.0f);
    }
}

static void UpdatePlayer(const Game& game, Player& player)
{
    UpdateInputs(game, player);

    for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURON_COUNT; inputIndex++)
    {
        const uint32_t spikeCount = player.InputState[inputIndex].Update(SIM_DT);
        if (spikeCount > 0)
            player.Network.TriggerConnected(inputIndex, spikeCount, PULSE_TIME / 2.0);
    }

    for (uint32_t i = 0; i < NEURON_SUBSTEPS; i++)
    {
        const double substepDeltaTime = SIM_DT / static_cast<double>(NEURON_SUBSTEPS);

        player.Network.UpdateNetwork(substepDeltaTime);
    }
}

static Individual StartTraining(const Renderer& renderer, Game& game, std::mt19937& rng)
{
    GameState gameState;

    std::vector<Individual> individuals;
    individuals.reserve(MAX_INDIVIDUALS);

    std::vector<double> history;
    double lastBestFitness = 0.0;

    auto startGeneration = [&]()
        {
            std::sort(individuals.begin(), individuals.end(), [&](const Individual& a, const Individual& b)
                {
                    return a.EvaluateFitness(game) > b.EvaluateFitness(game);
                });

            gameState.Generation += 1;
            std::cout << gameState.Generation << std::endl;
            Individual* lastBestIndividual = individuals.size() > 0 ? &individuals[0] : nullptr;
            if (lastBestIndividual)
            {
                lastBestIndividual->Genome.Print();
                std::cout << "Fitness: " << lastBestIndividual->EvaluateFitness(game) << std::endl;
            }

            const double alpha = std::clamp((static_cast<double>(gameState.Generation) - static_cast<double>(ALPHA_START)) / static_cast<double>(ALPHA_DURATION), 0.0, 1.0);

            const double bestFitness = lastBestIndividual ? lastBestIndividual->EvaluateFitness(game) : 0.0;

            std::cout << "Alpha: " << alpha << std::endl;

            lastBestFitness = bestFitness;

            if (individuals.size() > 0)
                history.emplace_back(bestFitness);

            game.Reset();

            std::vector<Individual> nextIndividuals;

            std::uniform_int_distribution<size_t> randomDistribution(0, EVOLUTION_MU - 1);
            std::uniform_real_distribution<float> continuousDistribution(0.0f, 1.0f);
            for (uint32_t individualIndex = 0; individualIndex < MAX_INDIVIDUALS; individualIndex++)
            {
                Individual& individual = nextIndividuals.emplace_back();

                if (gameState.Generation >= MULTI_EVAL_START)
                    individual.EvalutationsPerGenome = MAX_EVALUTIONS_PER_GENOME;

                if (individualIndex >= 1 and individuals.size() > 0)
                {
                    if (continuousDistribution(rng) < CROSSOVER_CHANCE)
                    {
                        const Genome& genomeA = individuals[std::min(randomDistribution(rng), individuals.size() - 1)].Genome;
                        const Genome& genomeB = individuals[std::min(randomDistribution(rng), individuals.size() - 1)].Genome;

                        individual.Genome = CrossoverGenome(genomeA, genomeB, rng);
                    }
                    else
                    {
                        individual.Genome = individuals[std::min(randomDistribution(rng), individuals.size() - 1)].Genome;
                    }

                    individual.Genome.Mutate(rng);

                    std::mt19937 playerRng(HashSeed(gameState.Generation, 0));
                    for (uint32_t i = 0; i < individual.EvalutationsPerGenome; i++)
                    {
                        Player& player = individual.Players2[i];
                        const uint32_t playerIndex = game.AddPlayer(false, playerRng);
                        player.PlayerIndex = playerIndex;
                    }
                }
                else
                {
                    if (individualIndex < individuals.size())
                        individual.Genome = individuals[individualIndex].Genome;

                    std::mt19937 playerRng(HashSeed(gameState.Generation, 0));
                    for (uint32_t i = 0; i < individual.EvalutationsPerGenome; i++)
                    {
                        Player& player = individual.Players2[i];
                        const uint32_t playerIndex = game.AddPlayer(true, playerRng);
                        player.PlayerIndex = playerIndex;
                    }
                }

                ConstructNetwork(individual);

                for (uint32_t playerIndex = 0; playerIndex < individual.EvalutationsPerGenome; playerIndex++)
                {
                    Player& player = individual.Players2[playerIndex];
                    player.Network = individual.BaseNetwork;

                    std::mt19937 slotRng(HashSeed(gameState.Generation, playerIndex));
                    VaryNetwork(player.Network, slotRng, alpha);
                }
            }

            // TODO: Move needed?
            individuals = std::move(nextIndividuals);
        };

    startGeneration();

    double lastFrameTime = 0.0;
    while (!gameState.Quit)
    {
        Frame frame = {};
        if (gameState.Render)
        {
            frame = FrameStart(renderer);
        }

        HandleGameInputs(gameState);
        game.Step(SIM_DT, gameState.Generation > STRICT_MODE_START);

        // New generation
        if (game.IsDone() || gameState.Skip || game.GetSimTime() > MAX_GAME_TIME)
        {
            gameState.Skip = false;
            startGeneration();
        }

        std::for_each(std::execution::par, individuals.begin(), individuals.end(), [&game](Individual& individual)
            {
                double fitness = individual.EvaluateFitness(game);

                for (uint32_t playerIndex = 0; playerIndex < individual.EvalutationsPerGenome; playerIndex++)
                {
                    auto& player = individual.Players2[playerIndex];
                    if (!game.PlayerAlive(player.PlayerIndex))
                    {
                        individual.Alive = false;
                        break;
                    }

                    UpdatePlayer(game, player);
                }
            });

        for (auto& individual : individuals)
        {
            if (!individual.Alive)
            {
                //for (auto& otherPlayer : individual.Players)
                    //game.KillPlayer(otherPlayer.PlayerIndex);
            }
            for (uint32_t i = 0; i < individual.EvalutationsPerGenome; i++)
            {
                Player& player = individual.Players2[i];
                for (uint32_t neuronIndex = 0; neuronIndex < player.Network.Neurons.size(); neuronIndex++)
                {
                    if (!player.Network.PendingTrigger[neuronIndex]) continue;
                    player.Network.PendingTrigger[neuronIndex] = false;

                    const int32_t outputIndex = neuronIndex - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT;
                    if (outputIndex >= 0 && outputIndex < OUTPUT_NEURON_COUNT)
                    {
                        if (!gameState.Disable)
                            game.Action(player.PlayerIndex, outputIndex);
                    }
                }
            }
        }

        if (gameState.Render)
        {
            game.Render();
            DrawSidebar(renderer.Renderer, gameState.Generation, individuals, game, history, 0.0, lastFrameTime);

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }

    if (individuals.size() > 0)
        return individuals[0];
    else
        return {};
}

static void StartSim(const Renderer& renderer, Game& game, std::mt19937& rng, Individual& simIndividual)
{
    GameState gameState;

    std::vector<double> history;

    simIndividual.BaseNetwork = {};
    ConstructNetwork(simIndividual);
    simIndividual.EvalutationsPerGenome = 1;

    // TODO: Bad RNG
    std::mt19937 playerRng(0);

    auto resetRound = [&]()
        {
            game.Reset();

            Player& player = simIndividual.Players2[0];
            player.PlayerIndex = game.AddPlayer(true, playerRng);
            player.Network = simIndividual.BaseNetwork;

            VaryNetwork(player.Network, playerRng, 1.0);

            for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURON_COUNT; inputIndex++)
                player.InputState[inputIndex].Reset();
        };

    resetRound();

    double lastFrameTime = 0.0;
    while (!gameState.Quit)
    {
        Frame frame = {};
        if (gameState.Render)
            frame = FrameStart(renderer);

        HandleGameInputs(gameState);
        game.Step(SIM_DT, false);

        if (game.IsDone() || gameState.Skip || game.GetSimTime() > MAX_GAME_TIME)
        {
            gameState.Skip = false;
            resetRound();
        }

        Player& player = simIndividual.Players2[0];

        if (game.PlayerAlive(player.PlayerIndex))
        {
            UpdatePlayer(game, player);

            for (uint32_t neuronIndex = 0; neuronIndex < player.Network.Neurons.size(); neuronIndex++)
            {
                if (!player.Network.PendingTrigger[neuronIndex]) continue;
                player.Network.PendingTrigger[neuronIndex] = false;

                const int32_t outputIndex = neuronIndex - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT;
                if (outputIndex >= 0 && outputIndex < OUTPUT_NEURON_COUNT)
                {
                    if (!gameState.Disable)
                        game.Action(player.PlayerIndex, outputIndex);
                }
            }
        }

        if (gameState.Render)
        {
            game.Render();

            std::vector<Individual> simIndividuals = { simIndividual };
            DrawSidebar(renderer.Renderer, gameState.Generation, simIndividuals, game, history, 0.0, lastFrameTime);

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }
}

static void StartExp(const Renderer& renderer, Game& game, std::mt19937& rng)
{
    GameState gameState;

    std::vector<double> history;

    Player player = {};

    serialib serial;
    serial.openDevice("COM3", 115200);

    std::string rxBuffer;

    std::mt19937 playerRng(0);

    auto resetRound = [&]()
        {
            game.Reset();

            player.PlayerIndex = game.AddPlayer(true, playerRng);

            for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURON_COUNT; inputIndex++)
                player.InputState[inputIndex].Reset();
        };

    resetRound();

    double lastFrameTime = 0.0;
    while (!gameState.Quit)
    {
        Frame frame = {};
        if (gameState.Render)
            frame = FrameStart(renderer);

        HandleGameInputs(gameState);
        game.Step(SIM_DT, false);

        if (game.IsDone() || gameState.Skip || game.GetSimTime() > MAX_GAME_TIME)
        {
            gameState.Skip = false;
            resetRound();
        }

        UpdateInputs(game, player);

        for (uint32_t serialIdx = 0; serialIdx < SERIAL_SUBSTEPS; serialIdx++)
        {
            const double substepDeltaTime = SIM_DT / static_cast<double>(SERIAL_SUBSTEPS);

            constexpr uint32_t bitCount = 8;
            std::bitset<bitCount> spikedMask(0);
            static_assert(INPUT_NEURON_COUNT <= bitCount);

            auto updateInput = [&](uint32_t inputIndex)
                {
                    const uint32_t spikeCount = player.InputState[inputIndex].Update(substepDeltaTime);
                    if (spikeCount > 1)
                    {
                        std::cout << "Critical error, SERIAL_SUBSTEPS is too low, skipping spikes.\n";
                    }
                    if (spikeCount > 0)
                    {
                        spikedMask[inputIndex] = true;
                    }
                };

            for (uint32_t inputIdx = 0; inputIdx < INPUT_NEURON_COUNT; inputIdx++)
                updateInput(inputIdx);
            
            if (serial.writeChar(static_cast<char>(spikedMask.to_ullong())) == -1)
            {
                std::cout << "Error writing to serial buffer, is the port connected?\n";
            }

            SerialReadToBuffer(serial, rxBuffer);

            for (uint32_t charIdx = 0; charIdx < rxBuffer.size(); charIdx++)
            {
                const uint32_t outputIdx = rxBuffer[charIdx];
                game.Action(player.PlayerIndex, outputIdx);
            }
        }

        if (gameState.Render)
        {
            game.Render();

            std::vector<Individual> simIndividuals;
            DrawSidebar(renderer.Renderer, gameState.Generation, simIndividuals, game, history, 0.0, lastFrameTime);

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }
}

int main(int argc, char* argv[])
{

    const Renderer renderer = StartSDL();
    std::mt19937 rng(std::random_device{}());
    CartPole game(renderer.Renderer);

#if 1
    Individual bestIndividual = StartTraining(renderer, game, rng);
    StartSim(renderer, game, rng, bestIndividual);
#else
    StartExp(renderer, game, rng);
#endif

    StopSDL(renderer);
    
    return 0;
}