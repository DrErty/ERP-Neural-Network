#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "serialib.h"

#include "Simulation/Drawer.h"
#include "Simulation/CartPole.h"

#include "Simulation/SimulationUI.h"

#include "Simulation/ContinuousNetwork.h"

#include "Simulation/Oscilloscope.h"
#include "Simulation/RingBuffer.h"

#include "Simulation/IO.h"

static constexpr const char* FILE_PATH = "Genome.csv";

struct Renderer
{
    SDL_Renderer* Renderer;
    SDL_Window* Window;
    int WindowWidth = Drawer::DEFAULT_WINDOW_WIDTH;
    int WindowHeight = Drawer::DEFAULT_WINDOW_HEIGHT;
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

static Scalar CalculateDeltaTime(Uint64 start, Uint64 end)
{
    Assert(end > start);
    return static_cast<Scalar>(end - start) / static_cast<Scalar>(SDL_GetPerformanceFrequency());
}

struct Frame
{
    Uint64 Start;
};

static Frame FrameStart(Renderer& renderer)
{
    SDL_GetWindowSize(renderer.Window, &renderer.WindowWidth, &renderer.WindowHeight);

    return { SDL_GetPerformanceCounter() };
}

static Scalar FrameEnd(const Renderer& renderer, const Frame& frame, bool vSync)
{
    SDL_RenderPresent(renderer.Renderer);

    {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        const Scalar elapsedTime = CalculateDeltaTime(frame.Start, currentTime);
        if ((elapsedTime < SIM_DT) && vSync)
            SDL_Delay(static_cast<Uint32>((SIM_DT - elapsedTime) * 1000.0));
    }

    Uint64 currentTimeAfterDelay = SDL_GetPerformanceCounter();
    const Scalar elapsedTimeAfterDelay = CalculateDeltaTime(frame.Start, currentTimeAfterDelay);

    return elapsedTimeAfterDelay;
}

struct GameState
{
    bool Quit = false;
    bool Skip = false;
    bool DisplayUI = true;

    bool Pause = false;
    bool VSync = true;
    bool Render = true;
    bool DisableInputs = false;
    bool TrackingCamera = true;

    uint32_t Generation = 0;
};

static void HandleGameInputs(SDL_Event event, GameState& gameState)
{
    if (event.type == SDL_EVENT_QUIT)
        gameState.Quit = true;
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.key == SDLK_ESCAPE)
            gameState.Quit = true;
        if (event.key.key == SDLK_P)
            gameState.Skip = true;
        if (event.key.key == SDLK_SPACE)
            gameState.DisplayUI = !gameState.DisplayUI;
    }
}

static void UpdateInputs(const CartPole& game, Player& player)
{
    for (uint32_t inputIdx = 0; inputIdx < INPUT_NEURON_COUNT; inputIdx++)
    {
        //Scalar input = game.GetInput(player.PlayerIndex, inputIdx);
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
        const Scalar substepDeltaTime = SIM_DT / static_cast<Scalar>(NEURON_SUBSTEPS);

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
    Scalar totalGameFitness = Scalar(0.0);
    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
    {
        const Scalar fitness = game.PlayerFitness(unit.PlayerIndices[idx]);
        totalGameFitness += fitness;
    }

    Scalar lowestFitness = std::numeric_limits<Scalar>::max();
    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
    {
        const Scalar fitness = game.PlayerFitness(unit.PlayerIndices[idx]);
        if (fitness < lowestFitness)
            lowestFitness = fitness;
    }

    constexpr Scalar alpha = Scalar(0.75);
    const Scalar gameFitness = alpha * lowestFitness + (Scalar(1.0) - alpha) * totalGameFitness / static_cast<Scalar>(MAX_EVALUTIONS_PER_GENOME);

    return gameFitness;
}

static void StartLoop(Renderer& renderer, GameState& gameState, CartPole& game,
    std::function<void(uint64_t frameIndex)> updateFunction,
    std::function<void()> renderFunction,
    std::function<bool()> resetFunction)
{
    SettingsPanel settingsPanel(renderer.Window);
    settingsPanel.AddToggle("Pause", gameState.Pause);
    settingsPanel.AddToggle("VSync", gameState.VSync);
    settingsPanel.AddToggle("Rendering Enabled (WARNING!)", gameState.Render);
    settingsPanel.AddToggle("Disable Inputs", gameState.DisableInputs);
    settingsPanel.AddToggle("Tracking Camera", gameState.TrackingCamera);

    resetFunction();

    Scalar lastFrameTime = Scalar(0.0);
    uint64_t frameIndex = 0;
    while (!gameState.Quit)
    {
        Frame frame = {};
        if (gameState.Render)
            frame = FrameStart(renderer);

        game.SetWindowSize(renderer.WindowWidth, renderer.WindowHeight);
        settingsPanel.SetWindowSize(renderer.WindowWidth, renderer.WindowHeight);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            HandleGameInputs(event, gameState);
            settingsPanel.HandleEvent(event);
        }

        if (!gameState.Pause)
        {
            game.Step(SIM_DT, gameState.TrackingCamera);
        }

        if (game.IsDone() || gameState.Skip || game.GetSimTime() > MAX_GAME_TIME)
        {
            gameState.Skip = false;
            if (resetFunction())
            {
                gameState.Quit = true;
                break;
            }
        }

        if (!gameState.Pause)
        {
            updateFunction(frameIndex);
        }

        frameIndex++;

        if (gameState.Render)
        {
            game.Render();

            if (gameState.DisplayUI)
            {
                renderFunction();

                settingsPanel.Draw(renderer.Renderer);
            }

            lastFrameTime = FrameEnd(renderer, frame, gameState.VSync);
        }
    }
}

struct MeasurementBuffers
{
    RingBuffer TimeBuffer;
    RingBuffer ThetaBuffer;
    RingBuffer ThetaDotBuffer;
    RingBuffer XBuffer;
    RingBuffer XDotBuffer;

    bool IsFull() const
    {
        return TimeBuffer.Full();
    }

    void Push(Scalar time, const CartPole::PhysicsState& state)
    {
        TimeBuffer.Push(time);
        ThetaBuffer.Push(state.Theta);
        ThetaDotBuffer.Push(state.ThetaDot);
        XBuffer.Push(state.X);
        XDotBuffer.Push(state.XDot);
    }

    void Clear()
    {
        TimeBuffer.Clear();
        ThetaBuffer.Clear();
        ThetaDotBuffer.Clear();
        XBuffer.Clear();
        XDotBuffer.Clear();
    }

    void SaveToFile(std::string_view filePath, uint32_t generation)
    {
        if (IsFull())
        {
            StaticBuffer<char> buffer(1024);
            StringBuilder stringBuilder(buffer);
            stringBuilder.Concat(filePath);
            stringBuilder.Concat(generation);
            stringBuilder.Concat(".txt");
            stringBuilder.Compile();

            IO::SaveCsv(buffer.GetData(), {"Time", "Theta", "ThetaDot", "X", "XDot"}, TimeBuffer.Span(), ThetaBuffer.Span(), ThetaDotBuffer.Span(), XBuffer.Span(), XDotBuffer.Span());
            ERP_LOG("Saving file to: ", buffer.GetData());
        }
        else
        {
            ERP_LOG("Skipping saving to file, buffer not full.");
        }
    }
};

static void StartTrainingBetter(Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    MeasurementBuffers buffers;

    std::vector<EvolutionUnit> units;
    units.reserve(MAX_INDIVIDUALS);

    std::vector<Scalar> history;

    auto resetFunction = [&]() -> bool
        {
            buffers.Clear();

            std::sort(units.begin(), units.end(), [&](const EvolutionUnit& a, const EvolutionUnit& b)
                {
                    return ComputeFitness(game, a) > ComputeFitness(game, b);
                });

            gameState.Generation += 1;
            ERP_LOG(gameState.Generation);
            EvolutionUnit* lastBestUnit = units.size() > 0 ? &units[0] : nullptr;
            if (lastBestUnit)
            {
                ERP_LOG("Fitness: ", ComputeFitness(game, *lastBestUnit));
            }

            const Scalar bestFitness = lastBestUnit ? ComputeFitness(game, *lastBestUnit) : Scalar(0.0);
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
                    const uint32_t playerIndex = game.AddPlayer(unitIndex == 0, playerRng, 0);
                    unit.PlayerIndices[idx] = playerIndex;

                    unit.Networks[idx].SetFromGenome(unit.Genome);
                }
            }

            // TODO: Move needed?
            units = std::move(nextUnits);

            return false;
        };

    auto updateFunction = [&](uint64_t frameIndex)
        {
            std::for_each(std::execution::par, units.begin(), units.end(), [&game, &gameState](EvolutionUnit& unit)
                {
                    for (uint32_t idx = 0; idx < MAX_EVALUTIONS_PER_GENOME; idx++)
                    {
                        const std::array<Scalar, INPUT_COUNT> input = game.GetInputs(unit.PlayerIndices[idx]);
                        std::array<Scalar, OUTPUT_COUNT> output = {};
                        unit.Networks[idx].Evaluate(input, output);

                        if (!gameState.DisableInputs)
                            game.SetForce(unit.PlayerIndices[idx], output[0]);
                    }
                });

            if (units.size() > 0)
            {
                const CartPole::PhysicsState& state = game.GetState(units[0].PlayerIndices[0]);
                buffers.Push(game.GetSimTime(), state);
            }
        };
    
    auto renderFunction = [&]()
        {
            dashboard.SetWindowSize(renderer.WindowWidth, renderer.WindowHeight);

            dashboard.DrawScope(renderer.Renderer, buffers.ThetaBuffer.Span());

            if (units.size() > 0)
            {
                dashboard.DrawNetwork(renderer.Renderer, units[0].Genome);
            }
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);

    if (units.size() > 0)
    {
        IO::SaveGenome(units[0].Genome, FILE_PATH);
        ERP_LOG("Saving genome to file at: ", FILE_PATH);
    }
}

static bool SharedResetFunction(GameState& gameState, CartPole& game, MeasurementBuffers& buffers, uint32_t& currentPlayerIndex, std::mt19937& playerRng, std::string_view filePath)
{
    if (gameState.Generation <= MAX_MEASUREMENTS)
        buffers.SaveToFile(filePath, gameState.Generation);

    buffers.Clear();
    game.Reset();

    if (gameState.Generation >= MAX_MEASUREMENTS)
        return true;

    currentPlayerIndex = game.AddPlayer(true, playerRng, gameState.Generation);

    gameState.Generation += 1;

    return false;
}

static void StartSim(Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    MeasurementBuffers buffers;

    IO::GenomeLoadResult genomeLoadResult = IO::LoadGenome(FILE_PATH);
    if (!genomeLoadResult)
    {
        ERP_LOG("Error loading genome file: ", genomeLoadResult.Error);
        return;
    }

    ContinuousNetwork network(genomeLoadResult.Genome);
    uint32_t currentPlayerIndex = 0;

    // TODO: Bad RNG
    std::mt19937 playerRng(0);

    auto resetFunction = [&]() -> bool
        {
            return SharedResetFunction(gameState, game, buffers, currentPlayerIndex, playerRng, "Data\\SIM\\SwingUpMeting");
        };

    auto updateFunction = [&](uint64_t frameIndex)
        {
            const std::array<Scalar, INPUT_COUNT> input = game.GetInputs(currentPlayerIndex);
            std::array<Scalar, OUTPUT_COUNT> output = {};
            network.Evaluate(input, output);

            if (!gameState.DisableInputs)
                game.SetForce(currentPlayerIndex, output[0]);

            const CartPole::PhysicsState& state = game.GetState(currentPlayerIndex);
            if (!buffers.IsFull())
            {
                buffers.Push(game.GetSimTime(), state);
            }
            else
            {
                resetFunction();
            }
        };

    auto renderFunction = [&]()
        {
            dashboard.SetWindowSize(renderer.WindowWidth, renderer.WindowHeight);

            dashboard.DrawScope(renderer.Renderer, buffers.ThetaBuffer.Span());
            dashboard.DrawNetwork(renderer.Renderer, genomeLoadResult.Genome);
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);
}

static void StartExp(Renderer& renderer, GameState& gameState, CartPole& game, std::mt19937& rng)
{
    Dashboard dashboard;

    MeasurementBuffers buffers;

    uint32_t currentPlayerIndex = 0;

    // TODO: Bad RNG
    std::mt19937 playerRng(0);

    serialib serial;
    serial.openDevice("COM3", 115200);

    auto resetFunction = [&]() -> bool
        {
            return SharedResetFunction(gameState, game, buffers, currentPlayerIndex, playerRng, "Data\\EXP\\SwingUpMeting");
        };

    auto updateFunction = [&](uint64_t frameIndex)
        {
            const std::array<Scalar, INPUT_COUNT> inputs = game.GetInputs(currentPlayerIndex);

            {
                StaticBuffer<char> serialString(1024);
                StringBuilder stringBuilder(serialString);
                stringBuilder.Concat(inputs[0]);
                stringBuilder.Concat(',');
                stringBuilder.Concat(inputs[1]);
                stringBuilder.Concat(',');
                stringBuilder.Concat(inputs[2]);
                stringBuilder.Concat('\n');
                stringBuilder.Compile();

                serial.writeString(serialString.GetData());
            }

            StaticBuffer<char> buffer(1024);
            while (serial.available() > 0)
            {
                serial.readString(buffer.GetData(), '\n', static_cast<uint32_t>(buffer.GetCount() - 1));

                if (strncmp(buffer.GetData(), "Error", 5) == 0)
                {
                    //ERP_LOG(buffer.GetData());
                }
                else
                {
                    Scalar out = Scalar(0.0);
                    const auto [ptr, ec] = std::from_chars(buffer.GetData(), buffer.GetData() + buffer.GetCount(), out);

                    if (!gameState.DisableInputs)
                        game.SetForce(currentPlayerIndex, out);

                    //ERP_LOG(out);
                }
            }

            const CartPole::PhysicsState& state = game.GetState(currentPlayerIndex);
            if (!buffers.IsFull())
            {
                buffers.Push(game.GetSimTime(), state);
            }
            else
            {
                resetFunction();
            }
        };

    auto renderFunction = [&]()
        {
            dashboard.SetWindowSize(renderer.WindowWidth, renderer.WindowHeight);

            dashboard.DrawScope(renderer.Renderer, buffers.ThetaBuffer.Span());
        };

    StartLoop(renderer, gameState, game, updateFunction, renderFunction, resetFunction);
}

int main(int argc, char* argv[])
{
    Memory::ThreadStackAllocator.AllocateBlock(static_cast<size_t>(8) * 1024 * 1024);

    Renderer renderer = StartSDL();
    std::mt19937 rng(std::random_device{}());
    CartPole game(renderer.Renderer);

    GameState gameState;

    //StartTrainingBetter(renderer, gameState, game, rng);
    StartSim(renderer, gameState, game, rng);
    //StartExp(renderer, gameState, game, rng);

    StopSDL(renderer);
    
    Memory::ThreadStackAllocator.DeallocateBlock();

    return 0;
}