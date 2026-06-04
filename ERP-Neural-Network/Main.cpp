#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "serialib.h"

#include "Drawer.h"
#include "CartPole.h"

#include "SimulationUI.h"

#include "ContinuousNetwork.h"

#include "Oscilloscope.h"
#include "RingBuffer.h"

#include "IO.h"

static constexpr const char* FILE_PATH = "Genome.csv";

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

    SDL_Window* window = SDL_CreateWindow("Evolutionary Training", Drawer::DEFAULT_WINDOW_WIDTH, Drawer::DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
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

static void UpdateInputs(const CartPole& game, Player& player)
{
    for (uint32_t inputIdx = 0; inputIdx < INPUT_NEURON_COUNT; inputIdx++)
    {
        //double input = game.GetInput(player.PlayerIndex, inputIdx);
        //player.InputState[inputIdx].SetValue(input);
    }
}

static void UpdatePlayer(const CartPole& game, Player& player)
{
    UpdateInputs(game, player);

    for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURON_COUNT; inputIndex++)
    {
        const uint32_t spikeCount = player.InputState[inputIndex].Update(SIM_DT);
        if (spikeCount > 0)
            player.Network.TriggerConnected(inputIndex, spikeCount);
    }

    for (uint32_t i = 0; i < NEURON_SUBSTEPS; i++)
    {
        const double substepDeltaTime = SIM_DT / static_cast<double>(NEURON_SUBSTEPS);

        player.Network.UpdateNetwork(substepDeltaTime);
    }
}

struct EvolutionUnit
{
    NetworkGenome Genome = {};
    std::array<ContinuousNetwork, MAX_EVALUTIONS_PER_GENOME> Networks = {};
    std::array<uint32_t, MAX_EVALUTIONS_PER_GENOME> PlayerIndices = {};
};

static Scalar ComputeFitness(const CartPole& game, const EvolutionUnit& unit)
{
    double totalGameFitness = 0.0;
    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
    {
        const double fitness = game.PlayerFitness(unit.PlayerIndices[idx]);
        totalGameFitness += fitness;
    }

    double lowestFitness = std::numeric_limits<double>::max();
    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
    {
        const double fitness = game.PlayerFitness(unit.PlayerIndices[idx]);
        if (fitness < lowestFitness)
            lowestFitness = fitness;
    }

    constexpr double alpha = 0.75;
    const double gameFitness = alpha * lowestFitness + (1.0 - alpha) * totalGameFitness / static_cast<double>(MAX_EVALUTIONS_PER_GENOME);

    return gameFitness;
}

static void StartLoop(const Renderer& renderer, GameState& gameState, CartPole& game,
    std::function<void(uint64_t frameIndex)> updateFunction,
    std::function<void()> renderFunction,
    std::function<void()> resetFunction)
{
    resetFunction();

    double lastFrameTime = 0.0;
    uint64_t frameIndex = 0;
    while (!gameState.Quit)
    {
        Frame frame = {};
        if (gameState.Render)
            frame = FrameStart(renderer);

        HandleGameInputs(gameState);
        game.Step(SIM_DT);

        if (game.IsDone() || gameState.Skip || game.GetSimTime() > MAX_GAME_TIME)
        {
            gameState.Skip = false;
            resetFunction();
        }

        updateFunction(frameIndex);
        frameIndex++;

        if (gameState.Render)
        {
            game.Render();

            renderFunction();

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }
}

static void StartTrainingBetter(const Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    RingBuffer thetaBuffer;

    std::vector<EvolutionUnit> units;
    units.reserve(MAX_INDIVIDUALS);

    std::vector<double> history;

    auto resetFunction = [&]()
        {
            thetaBuffer.Clear();

            std::sort(units.begin(), units.end(), [&](const EvolutionUnit& a, const EvolutionUnit& b)
                {
                    return ComputeFitness(game, a) > ComputeFitness(game, b);
                });

            gameState.Generation += 1;
            std::cout << gameState.Generation << std::endl;
            EvolutionUnit* lastBestUnit = units.size() > 0 ? &units[0] : nullptr;
            if (lastBestUnit)
            {
                //lastBestUnit->Genome.Print();
                std::cout << "Fitness: " << ComputeFitness(game, *lastBestUnit) << std::endl;
            }

            const double bestFitness = lastBestUnit ? ComputeFitness(game, *lastBestUnit) : 0.0;
            if (units.size() > 0)
                history.emplace_back(bestFitness);

            game.Reset();

            std::vector<EvolutionUnit> nextUnits;

            std::uniform_int_distribution<size_t> randomDistribution(0, EVOLUTION_MU - 1);
            std::uniform_real_distribution<float> continuousDistribution(0.0f, 1.0f);
            for (uint32_t unitIndex = 0; unitIndex < MAX_INDIVIDUALS; unitIndex++)
            {
                EvolutionUnit& unit = nextUnits.emplace_back();

                if (unitIndex >= 1 and units.size() > 0)
                {
                    if (continuousDistribution(rng) < CROSSOVER_CHANCE)
                    {
                        const NetworkGenome& genomeA = units[std::min(randomDistribution(rng), units.size() - 1)].Genome;
                        const NetworkGenome& genomeB = units[std::min(randomDistribution(rng), units.size() - 1)].Genome;

                        unit.Genome = Crossover(genomeA, genomeB, rng);
                    }
                    else
                    {
                        unit.Genome = units[std::min(randomDistribution(rng), units.size() - 1)].Genome;
                    }

                    unit.Genome.Mutate(rng);
                }
                else
                {
                    if (unitIndex < units.size())
                        unit.Genome = units[unitIndex].Genome;
                }

                std::mt19937 playerRng(HashSeed(gameState.Generation, 0));
                for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
                {
                    const uint32_t playerIndex = game.AddPlayer(unitIndex == 0, playerRng);
                    unit.PlayerIndices[idx] = playerIndex;

                    unit.Networks[idx].SetFromGenome(unit.Genome);
                }
            }

            // TODO: Move needed?
            units = std::move(nextUnits);
        };

    auto updateFunction = [&](uint64_t frameIndex)
        {
            std::for_each(std::execution::par, units.begin(), units.end(), [&game, &gameState](EvolutionUnit& unit)
                {
                    static thread_local std::mt19937 threadLocalRng;

                    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
                    {
                        const std::array<Scalar, INPUT_COUNT> input = game.GetInputs(unit.PlayerIndices[idx]);
                        std::array<Scalar, OUTPUT_COUNT> output = {};
                        unit.Networks[idx].Evaluate(input, output);
                        game.SetForce(unit.PlayerIndices[idx], output[0]);
                    }
                });

            if (units.size() > 0)
            {
                const CartPole::PhysicsState& state = game.GetState(units[0].PlayerIndices[0]);
                thetaBuffer.Push(state.Theta);
            }
        };
    
    auto renderFunction = [&]()
        {
            dashboard.DrawScope(renderer.Renderer, thetaBuffer.Span());

            if (units.size() > 0)
            {
                dashboard.DrawNetwork(renderer.Renderer, units[0].Genome);
            }
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);

    if (units.size() > 0)
        IO::SaveGenome(units[0].Genome, FILE_PATH);
}

static void StartSim(const Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    RingBuffer thetaBuffer;

    IO::GenomeLoadResult genomeLoadResult = IO::LoadGenome(FILE_PATH);
    if (!genomeLoadResult)
    {
        // TODO: Replace with log function
        std::cout << "Error loading genome file: " << genomeLoadResult.Error << '\n';
        return;
    }

    ContinuousNetwork network(genomeLoadResult.Genome);
    uint32_t currentPlayerIndex = 0;

    // TODO: Bad RNG
    std::mt19937 playerRng(0);

    auto resetFunction = [&]()
        {
            thetaBuffer.Clear();

            game.Reset();

            currentPlayerIndex = game.AddPlayer(true, playerRng);
        };

    resetFunction();

    auto updateFunction = [&](uint64_t frameIndex)
        {
            const std::array<Scalar, INPUT_COUNT> input = game.GetInputs(currentPlayerIndex);
            std::array<Scalar, OUTPUT_COUNT> output = {};
            network.Evaluate(input, output);
            //if (frameIndex % 6 == 0)
                game.SetForce(currentPlayerIndex, output[0]);

            const CartPole::PhysicsState& state = game.GetState(currentPlayerIndex);
            thetaBuffer.Push(state.Theta);
        };

    auto renderFunction = [&]()
        {
            dashboard.DrawScope(renderer.Renderer, thetaBuffer.Span());
            dashboard.DrawNetwork(renderer.Renderer, genomeLoadResult.Genome);
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);
}

static void StartExp(const Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    RingBuffer thetaBuffer;

    uint32_t currentPlayerIndex = 0;

    // TODO: Bad RNG
    std::mt19937 playerRng(0);

    serialib serial;
    serial.openDevice("COM4", 115200);

    auto resetFunction = [&]()
        {
            thetaBuffer.Clear();

            game.Reset();

            currentPlayerIndex = game.AddPlayer(true, playerRng);
        };

    resetFunction();

    auto updateFunction = [&](uint64_t frameIndex)
        {
            const std::array<Scalar, INPUT_COUNT> inputs = game.GetInputs(currentPlayerIndex);
            std::string serialData;
            serialData += std::to_string(inputs[0]);
            serialData += ',';
            serialData += std::to_string(inputs[1]);
            serialData += ',';
            serialData += std::to_string(inputs[2]);
            serialData += '\n';
            serial.writeString(serialData.c_str());

            std::array<char, 1024> buff = {};
            while (serial.available() > 0)
            {
                // TODO: -1 needed?
                serial.readString(buff.data(), '\n', buff.size() - 1);

                if (strncmp(buff.data(), "Error", 5) == 0)
                {
                    std::cout << buff.data();
                }
                else
                {
                    double out = 0.0;
                    const auto [ptr, ec] = std::from_chars(buff.data(), buff.data() + buff.size(), out);

                    game.SetForce(currentPlayerIndex, out);
                    //std::cout << out << '\n';
                }
            }

            const CartPole::PhysicsState& state = game.GetState(currentPlayerIndex);
            thetaBuffer.Push(state.Theta);
        };

    auto renderFunction = [&]()
        {
            dashboard.DrawScope(renderer.Renderer, thetaBuffer.Span());
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);
}

/*
static void StartExp(const Renderer& renderer, CartPole& game, std::mt19937& rng)
{
    std::vector<double> history;

    Player player = {};

    serialib serial;
    serial.openDevice("COM3", 115200);

    std::string rxBuffer;

    std::mt19937 playerRng(0);

    double timeStart = 0.0;
    auto resetRound = [&]()
        {
            timeStart = 0.0;

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

        timeStart += SIM_DT;

        if (timeStart > 1.0)
            game.Step(SIM_DT);

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
            if (timeStart > 1.0)
                for (uint32_t charIdx = 0; charIdx < rxBuffer.size(); charIdx++)
                {
                    const uint32_t outputIdx = rxBuffer[charIdx];
                    std::cout << "Trigger: " << outputIdx << '\n';
                    //if (outputIdx == 1)
                    //game.Action(player.PlayerIndex, outputIdx - 23);
                }

            rxBuffer.clear();
        }

        if (gameState.Render)
        {
            game.Render();

            std::vector<Individual> simIndividuals;
            //DrawSidebar(renderer.Renderer, gameState.Generation, simIndividuals, game, history, 0.0, lastFrameTime);

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }
}
*/

int main(int argc, char* argv[])
{

    const Renderer renderer = StartSDL();
    std::mt19937 rng(std::random_device{}());
    CartPole game(renderer.Renderer);

    GameState gameState;

    //StartTrainingBetter(renderer, gameState, game, rng);
    //StartSim(renderer, gameState, game, rng);
    StartExp(renderer, gameState, game, rng);

    StopSDL(renderer);
    
    return 0;
}