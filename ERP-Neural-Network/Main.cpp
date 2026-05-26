#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <SDL3_ttf/SDL_ttf.h>
#include "serialib.h"

#include "Drawer.h"
#include "FlappyBird.h"
#include "CartPole.h"
#include "Neuron.h"
#include "Evolution.h"

static constexpr double SIM_DT = 1.0 / 60.0; // Seconds
static constexpr uint32_t NEURON_SUBSTEPS = 10;

static constexpr float M_PI = 3.14159265f;
static constexpr float NEURON_SIZE = 80.0f;
static constexpr float METER_WIDTH = NEURON_SIZE;
static constexpr float METER_HEIGHT = 16.0f;
static constexpr float METER_GAP = 8.0f;

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

static void DrawSidebar(SDL_Renderer* renderer, uint32_t generation, const std::vector<Individual>& individuals, Game& game, const std::vector<double>& history, double sigma, double frameTime)
{
    const double bestEver = history.size() > 0 ? *std::max_element(history.begin(), history.end()) : 0.0;

    double bestAliveFitness = 0.0;

    uint32_t m_GameW = Drawer::DEFAULT_WINDOW_WIDTH - Drawer::SIDEBAR_WIDTH;
    uint32_t m_GameH = Drawer::DEFAULT_WINDOW_HEIGHT;

    uint32_t m_WinW = Drawer::DEFAULT_WINDOW_WIDTH;
    uint32_t m_WinH = Drawer::DEFAULT_WINDOW_HEIGHT;

    const float sidebarX = (float)m_GameW;
    const float posX = sidebarX + 18.0f;
    float posY = 14.0f;
    const float barWidth = Drawer::SIDEBAR_WIDTH - 36.0f;

    Drawer::SetColor(renderer, { 8, 10, 18, 255 });
    Drawer::FillRect(renderer, sidebarX, 0.0f, Drawer::SIDEBAR_WIDTH, (float)m_WinH);
    Drawer::SetColor(renderer, { 40, 50, 80, 255 });
    SDL_RenderLine(renderer, sidebarX, 0.0f, sidebarX, (float)m_WinH);

    Drawer::DrawTextSlow(renderer, "NEURO FLAPPY", posX, posY, { 180, 200, 255, 220 }, Drawer::g_FontMedium);
    posY += 28.0f;

    Drawer::DrawTextSlow(renderer, "Generation", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
    posY += 20.0f;

    Drawer::DrawTextSlow(renderer, std::to_string(generation), posX, posY, { 255, 220, 60, 255 }, Drawer::g_FontMedium);
    posY += 20.0f;

    Drawer::DrawTextSlow(renderer, "Alive", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
    posY += 20.0f;

    uint32_t individualsAlive = 0;
    for (auto& individual : individuals)
    {
        if (individual.Alive)
        {
            individualsAlive += 1;
        }
    }

    const float aliveFraction = individualsAlive > 0 ? static_cast<float>(individualsAlive) / static_cast<float>(MAX_INDIVIDUALS) : 0.0f;

    Drawer::SetColor(renderer, { 25, 30, 55, 255 });
    Drawer::FillRect(renderer, posX, posY, barWidth, 20.0f);
    Drawer::SetColor(renderer, Drawer::LerpCol({ 220, 60, 60, 255 }, { 60, 220, 120, 255 }, aliveFraction));
    Drawer::FillRect(renderer, posX, posY, barWidth * aliveFraction, 20.0f);
    Drawer::SetColor(renderer, { 15, 18, 35, 255 });
    for (uint32_t tickIndex = 1; tickIndex < 10; ++tickIndex)
        Drawer::FillRect(renderer, posX + barWidth * (float)tickIndex * 0.1f - 0.5f, posY, 1.0f, 20.0f);
    posY += 20.0f;
    Drawer::DrawTextSlow(renderer, std::to_string(individualsAlive) + " / " + std::to_string(MAX_INDIVIDUALS), posX, posY, {160, 200, 255, 220}, Drawer::g_FontSmall);
    posY += 20.0f;

    Drawer::DrawTextSlow(renderer, "Best fitness", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
    posY += 20.0f;
    std::array<char, 128> textBuffer = {};
    std::snprintf(textBuffer.data(), textBuffer.size(), "%.1f", bestAliveFitness);
    Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 120, 255, 160, 255 }, Drawer::g_FontMedium);
    posY += 20.0f;

    Drawer::DrawTextSlow(renderer, "All-time best", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
    posY += 20.0f;
    std::snprintf(textBuffer.data(), textBuffer.size(), "%.1f", bestEver);
    Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 255, 220, 60, 200 }, Drawer::g_FontSmall);
    posY += 20.0f;

    {
        Drawer::DrawTextSlow(renderer, "Sigma", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;

        const float sigmaFraction = std::clamp((float)sigma / static_cast<float>(INITIAL_SIGMA), 0.0f, 1.0f);

        Drawer::SetColor(renderer, { 25, 30, 55, 255 });
        Drawer::FillRect(renderer, posX, posY, barWidth, 12.0f);
        Drawer::SetColor(renderer, Drawer::LerpCol({ 60, 120, 255, 180 }, { 255, 120, 60, 180 }, sigmaFraction));
        Drawer::FillRect(renderer, posX, posY, barWidth * sigmaFraction, 12.0f);

        std::snprintf(textBuffer.data(), textBuffer.size(), "%.3f", sigma);
        posY += 20.0f;
        Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 160, 160, 180, 180 }, Drawer::g_FontSmall);
        posY += 20.0f;
    }
    auto addPhase = [&](float phase, float min, float max)
        {
            std::snprintf(textBuffer.data(), textBuffer.size(), "Phase: %.3f, Min: %.3f, Max: %.3f", phase, min, max);
            Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
            posY += 10.0f;

            const float phaseFraction = std::clamp(phase, 0.0f, 1.0f);

            Drawer::SetColor(renderer, { 25, 30, 55, 255 });
            Drawer::FillRect(renderer, posX, posY, barWidth, 12.0f);
            Drawer::SetColor(renderer, Drawer::LerpCol({ 60, 120, 255, 180 }, { 255, 120, 60, 180 }, phaseFraction));
            Drawer::FillRect(renderer, posX, posY, barWidth * phaseFraction, 12.0f);

            Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 160, 160, 180, 180 }, Drawer::g_FontSmall);
            posY += 10.0f;
        };

    //const auto& spikeEncoderRange = bestIndividual.Genome.SpikeEncoderRange;

    for (uint32_t i = 0; i < INPUT_NEURONS; i++)
    {
        //addPhase(bestIndividual.Players[0].InputState[i].GetPhase(), spikeEncoderRange[i].Min, spikeEncoderRange[i].Max);
    }

    if (history.size() > 1)
    {
        Drawer::DrawTextSlow(renderer, "Fitness history", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;
        const float sparkHeight = 90.0f;
        Drawer::SetColor(renderer, { 20, 25, 48, 255 });
        Drawer::FillRect(renderer, posX, posY, barWidth, sparkHeight);
        Drawer::DrawRect(renderer, posX, posY, barWidth, sparkHeight);

        double maxFitness = *std::max_element(history.begin(), history.end());
        if (maxFitness < 1.0)
            maxFitness = 1.0;

        const uint32_t sampleCount = history.size();

        for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            const uint32_t historyIndex = (uint32_t)history.size() - sampleCount + sampleIndex;
            const float sparkX = posX + (float)sampleIndex * barWidth / (float)sampleCount;
            const float sparkY = posY + sparkHeight - (float)(history[historyIndex] / maxFitness * sparkHeight);
            const float alpha = 0.3f + 0.7f * (float)sampleIndex / (float)sampleCount;
            Drawer::SetColor(renderer, { 60, 160, 100, static_cast<uint8_t>(50.0f * alpha) });
            SDL_RenderLine(renderer, sparkX, sparkY, sparkX, posY + sparkHeight);
        }
        for (uint32_t sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex)
        {
            const uint32_t historyIndex = (uint32_t)history.size() - sampleCount + sampleIndex;
            const float startX = posX + (float)(sampleIndex - 1) * barWidth / (float)sampleCount;
            const float endX = posX + (float)sampleIndex * barWidth / (float)sampleCount;
            const float startY = posY + sparkHeight - (float)(std::max(history[historyIndex - 1], 0.0) / maxFitness * sparkHeight);
            const float endY = posY + sparkHeight - (float)(std::max(history[historyIndex], 0.0) / maxFitness * sparkHeight);
            Drawer::SetColor(renderer, { 100, 220, 160, 220 });
            SDL_RenderLine(renderer, startX, startY, endX, endY);
        }
        const float bestLine = posY + sparkHeight - (float)(bestEver / maxFitness * sparkHeight);
        Drawer::SetColor(renderer, { 255, 220, 60, 100 });
        SDL_RenderLine(renderer, posX, bestLine, posX + barWidth, bestLine);
        posY += sparkHeight + 12.0f;
    }

    Drawer::SetColor(renderer, { 60, 70, 100, 200 });
    Drawer::FillRect(renderer, sidebarX, (float)m_WinH - 36.0f, Drawer::SIDEBAR_WIDTH, 36.0f);
    Drawer::DrawTextSlow(renderer, "[ESC] quit   [R] pause", posX, (float)m_WinH - 26.0f,
        { 120, 130, 160, 200 }, Drawer::g_FontSmall);
    Drawer::DrawTextSlow(renderer, "Frame time: " + std::to_string(frameTime * 1000.0) + "ms", posX, (float)m_WinH - 52.0f,
        { 120, 130, 160, 200 }, Drawer::g_FontSmall);

    {
        const NeuralNetwork& baseNetwork = individuals[0].Players[0].Network;

        constexpr uint32_t layerCount = 3;
        constexpr std::array<uint32_t, layerCount> layerSizes = { INPUT_NEURONS, HIDDEN_NEURONS, OUTPUT_NEURONS };
        constexpr std::array<uint32_t, layerCount> layerOffsets = { 0, INPUT_NEURONS, INPUT_NEURONS + HIDDEN_NEURONS };

        const float neuronRadius = 4.0f;
        const float networkHeight = 30.0f + 20.0f * TOTAL_NEURONS + 20.0f;

        Drawer::SetColor(renderer, { 10, 12, 20, 210 });
        Drawer::FillRect(renderer, posX, posY, barWidth, networkHeight);
        Drawer::SetColor(renderer, { 60, 80, 120, 255 });
        Drawer::DrawRect(renderer, posX, posY, barWidth, networkHeight);

        Drawer::SetColor(renderer, { 30, 40, 80, 255 });
        Drawer::FillRect(renderer, posX, posY, barWidth, 18.0f);
        Drawer::DrawTextSlow(renderer, "Neural network", posX + 4.0f, posY + 2.0f, { 180, 200, 255, 200 }, Drawer::g_FontSmall);

        const float panelTop = posY + 22.0f;
        const float panelBottom = posY + networkHeight - 6.0f;
        const float panelHeight = panelBottom - panelTop;

        const std::array<float, layerCount> layerX = { posX + barWidth * 0.18f, posX + barWidth * 0.50f, posX + barWidth * 0.82f };

        std::array<float, TOTAL_NEURONS> neuronScreenX;
        std::array<float, TOTAL_NEURONS> neuronScreenY;
        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
            const uint32_t neuronsInLayer = layerSizes[layerIndex];
            for (uint32_t positionInLayer = 0; positionInLayer < neuronsInLayer; ++positionInLayer)
            {
                const uint32_t neuronIndex = layerOffsets[layerIndex] + positionInLayer;
                neuronScreenX[neuronIndex] = layerX[layerIndex];
                neuronScreenY[neuronIndex] = panelTop + panelHeight * static_cast<float>(positionInLayer + 1) / static_cast<float>(neuronsInLayer + 1);
            }
        }

        Drawer::SetColor(renderer, { 40, 50, 80, 100 });
        const float midX01 = (layerX[0] + layerX[1]) * 0.5f;
        const float midX12 = (layerX[1] + layerX[2]) * 0.5f;
        SDL_RenderLine(renderer, midX01, panelTop, midX01, panelBottom);
        SDL_RenderLine(renderer, midX12, panelTop, midX12, panelBottom);

        Drawer::DrawTextSlow(renderer, "IN", layerX[0] - 8.0f, panelTop - 2.0f, { 100, 180, 255, 160 }, Drawer::g_FontSmall);
        Drawer::DrawTextSlow(renderer, "HID", layerX[1] - 8.0f, panelTop - 2.0f, { 60, 220, 120, 160 }, Drawer::g_FontSmall);
        Drawer::DrawTextSlow(renderer, "OUT", layerX[2] - 8.0f, panelTop - 2.0f, { 255, 180, 60, 160 }, Drawer::g_FontSmall);

        for (uint32_t postNeuronIndex = 0; postNeuronIndex < TOTAL_NEURONS; ++postNeuronIndex)
        {
            const Neuron& postNeuron = baseNetwork.Neurons[postNeuronIndex];
            for (uint32_t inputSlot = 0; inputSlot < MAX_INPUTS; ++inputSlot)
            {
                const int8_t preNeuronIndex = postNeuron.InputConnections[inputSlot];
                if (preNeuronIndex < 0) continue;

                const double weight = postNeuron.Weights[inputSlot];
                const float weightNormalised = std::clamp(static_cast<float>(weight / 1.5), -1.0f, 1.0f);
                const float weightAbsolute = std::abs(weightNormalised);

                Drawer::Col lineColor;
                if (weightNormalised >= 0.0f)
                    lineColor = Drawer::LerpCol({ 30, 30, 30, 30 }, { 60, 220, 100, 200 }, weightAbsolute);
                else
                    lineColor = Drawer::LerpCol({ 30, 30, 30, 30 }, { 220, 60, 60, 200 }, weightAbsolute);

                Drawer::SetColor(renderer, lineColor);

                const uint32_t lineCount = 1 + static_cast<uint32_t>(weightAbsolute * 2.0f);
                const float perpX = neuronScreenY[static_cast<uint32_t>(preNeuronIndex)] - neuronScreenY[postNeuronIndex];
                const float perpY = neuronScreenX[postNeuronIndex] - neuronScreenX[static_cast<uint32_t>(preNeuronIndex)];
                const float perpLength = std::sqrt(perpX * perpX + perpY * perpY) + 1e-6f;
                for (uint32_t lineIndex = 0; lineIndex < lineCount; ++lineIndex)
                {
                    const float offset = (lineCount == 1) ? 0.0f : (static_cast<float>(lineIndex) - static_cast<float>(lineCount - 1) * 0.5f) * 1.2f;
                    SDL_RenderLine(renderer, neuronScreenX[static_cast<uint32_t>(preNeuronIndex)] + offset * perpX / perpLength, neuronScreenY[static_cast<uint32_t>(preNeuronIndex)] + offset * perpY / perpLength, neuronScreenX[postNeuronIndex] + offset * perpX / perpLength, neuronScreenY[postNeuronIndex] + offset * perpY / perpLength);
                }

                const float labelX = (neuronScreenX[static_cast<uint32_t>(preNeuronIndex)] + neuronScreenX[postNeuronIndex]) * 0.5f + 2.0f;
                const float labelY = (neuronScreenY[static_cast<uint32_t>(preNeuronIndex)] + neuronScreenY[postNeuronIndex]) * 0.5f - 7.0f;
                std::array<char, 16> weightBuffer;
                std::snprintf(weightBuffer.data(), weightBuffer.size(), "%+.2f", weight);
                Drawer::DrawTextSlow(renderer, weightBuffer.data(), labelX, labelY, { 200, 200, 200, 180 }, Drawer::g_FontSmall);
            }
        }

        for (uint32_t neuronIndex = 0; neuronIndex < TOTAL_NEURONS; ++neuronIndex)
        {
            const Neuron& neuron = baseNetwork.Neurons[neuronIndex];
            const float activation = static_cast<float>(std::clamp(neuron.V_mem / neuron.V_threshold, 0.0, 1.0));

            Drawer::Col baseColor;
            if (neuronIndex < INPUT_NEURONS)
                baseColor = { 100, 180, 255, 255 };
            else if (neuronIndex < INPUT_NEURONS + HIDDEN_NEURONS)
                baseColor = { 60, 220, 120, 255 };
            else
                baseColor = { 255, 180, 60, 255 };

            const uint8_t brightness = static_cast<uint8_t>(55.0f + 200.0f * activation);
            const Drawer::Col dimColor = { static_cast<uint8_t>(static_cast<uint32_t>(baseColor.r) * brightness / 255), static_cast<uint8_t>(static_cast<uint32_t>(baseColor.g) * brightness / 255), static_cast<uint8_t>(static_cast<uint32_t>(baseColor.b) * brightness / 255), baseColor.a };

            Drawer::SetColor(renderer, { dimColor.r, dimColor.g, dimColor.b, static_cast<uint8_t>(40.0f * activation) });
            Drawer::FillRect(renderer, neuronScreenX[neuronIndex] - neuronRadius - 4.0f, neuronScreenY[neuronIndex] - neuronRadius - 4.0f, (neuronRadius + 4.0f) * 2.0f, (neuronRadius + 4.0f) * 2.0f);

            Drawer::SetColor(renderer, dimColor);
            Drawer::FillRect(renderer, neuronScreenX[neuronIndex] - neuronRadius, neuronScreenY[neuronIndex] - neuronRadius, neuronRadius * 2.0f, neuronRadius * 2.0f);

            Drawer::SetColor(renderer, { 200, 220, 255, 160 });
            Drawer::DrawRect(renderer, neuronScreenX[neuronIndex] - neuronRadius, neuronScreenY[neuronIndex] - neuronRadius, neuronRadius * 2.0f, neuronRadius * 2.0f);

            const float vmemBarWidth = neuronRadius * 2.0f;
            const float vmemBarHeight = 3.0f;
            const float vmemBarTop = neuronScreenY[neuronIndex] + neuronRadius + 3.0f;
            Drawer::SetColor(renderer, { 20, 28, 50, 200 });
            Drawer::FillRect(renderer, neuronScreenX[neuronIndex] - vmemBarWidth * 0.5f, vmemBarTop, vmemBarWidth, vmemBarHeight);
            Drawer::SetColor(renderer, dimColor);
            Drawer::FillRect(renderer, neuronScreenX[neuronIndex] - vmemBarWidth * 0.5f, vmemBarTop, vmemBarWidth * activation, vmemBarHeight);

            std::array<char, 8> neuronLabelBuffer;
            std::snprintf(neuronLabelBuffer.data(), neuronLabelBuffer.size(), "N%u", neuronIndex);
            Drawer::DrawTextSlow(renderer, neuronLabelBuffer.data(), neuronScreenX[neuronIndex] - neuronRadius, neuronScreenY[neuronIndex] - neuronRadius - 12.0f, { 160, 180, 220, 160 }, Drawer::g_FontSmall);
        }

        posY += networkHeight + 8.0f;
    }
}

static bool SerialReadToBuffer(serialib& serial, std::string& buffer)
{
    bool jump = false;
    while (serial.available() > 0)
    {
        char c;
        serial.readChar(&c, 0);
        if (c != '\r')
        {
            buffer += c;
        }

        if (c == '\n')
        {
            if (buffer == "Jump\n")
            {
                std::cout << buffer;
                jump = true;
            }
            buffer.clear();
        }
    }
    return jump;
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
    uint32_t Generation = 0;

    double Sigma = INITIAL_SIGMA;
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
        }
    }
}

static void StartTraining(const Renderer& renderer, Game& game, std::mt19937& rng)
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

            const double alpha = std::clamp(static_cast<double>(gameState.Generation) / 10.0 - 1.0, 0.0, 1.0);

            const double bestFitness = lastBestIndividual ? lastBestIndividual->EvaluateFitness(game) : 0.0;
            if (bestFitness > lastBestFitness * 0.9)
                gameState.Sigma *= 0.95;
            else
                gameState.Sigma /= 0.95;

            std::cout << "Sigma: " << gameState.Sigma << std::endl;
            std::cout << "Alpha: " << alpha << std::endl;

            lastBestFitness = bestFitness;

            gameState.Sigma = std::clamp(gameState.Sigma, 0.0, INITIAL_SIGMA);

            if (individuals.size() > 0)
                history.emplace_back(bestFitness);

            game.Reset();

            std::vector<Individual> nextIndividuals;

            std::uniform_int_distribution<int> randomDistribution(0, EVOLUTION_MU - 1);
            std::uniform_real_distribution<float> continuousDistribution(0.0f, 1.0f);
            for (uint32_t individualIndex = 0; individualIndex < MAX_INDIVIDUALS; individualIndex++)
            {
                Individual& individual = nextIndividuals.emplace_back();

                if (individuals.size() >= EVOLUTION_MU)
                {
                    if (individualIndex >= 1)
                    {
                        if (continuousDistribution(rng) < CROSSOVER_CHANCE)
                        {
                            const Genome& genomeA = individuals[randomDistribution(rng)].Genome;
                            const Genome& genomeB = individuals[randomDistribution(rng)].Genome;

                            individual.Genome = CrossoverGenome(genomeA, genomeB, rng);
                        }
                        else
                        {
                            individual.Genome = individuals[randomDistribution(rng)].Genome;
                        }

                        individual.Genome.Mutate(rng, gameState.Sigma);

                        for (auto& player : individual.Players)
                        {
                            uint32_t playerIndex = game.AddPlayer(false);
                            player.PlayerIndex = playerIndex;
                        }
                    }
                    else
                    {
                        individual.Genome = individuals[individualIndex].Genome;

                        for (auto& player : individual.Players)
                        {
                            uint32_t playerIndex = game.AddPlayer(true);
                            player.PlayerIndex = playerIndex;
                        }
                    }
                }
                else
                {
                    for (auto& player : individual.Players)
                    {
                        uint32_t playerIndex = game.AddPlayer(false);
                        player.PlayerIndex = playerIndex;
                    }
                }

                ConstructNetwork(individual);

                for (auto& player : individual.Players)
                {
                    player.Network = individual.BaseNetwork;
                    VaryNetwork(player.Network, rng, alpha);
                }
            }
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
        game.Step(SIM_DT);

        // New generation
        if (game.IsDone() || gameState.Skip || game.GetSimTime() > 30.0)
        {
            gameState.Skip = false;
            startGeneration();
        }

        std::for_each(std::execution::par, individuals.begin(), individuals.end(), [&game](Individual& individual)
            {
                double fitness = individual.EvaluateFitness(game);

                for (auto& player : individual.Players)
                {
                    if (!game.PlayerAlive(player.PlayerIndex))
                    {
                        individual.Alive = false;
                        break;
                    }

                    for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURONS; inputIndex++)
                    {
                        float input = game.GetInput(player.PlayerIndex, inputIndex);
                        player.InputState[inputIndex].SetValue(input, 0.0f, 1.0f);
                    }

                    for (uint32_t i = 0; i < NEURON_SUBSTEPS; i++)
                    {
                        const double substepDeltaTime = SIM_DT / static_cast<double>(NEURON_SUBSTEPS);

                        for (uint32_t inputIndex = 0; inputIndex < INPUT_NEURONS; inputIndex++)
                        {
                            bool spike = player.InputState[inputIndex].Update(substepDeltaTime);
                            if (spike)
                            {
                                player.Network.TriggerConnected(inputIndex);
                            }
                        }

                        UpdateNetwork(player.Network, substepDeltaTime);
                    }
                }
            });

        for (auto& individual : individuals)
        {
            if (!individual.Alive)
            {
                //for (auto& otherPlayer : individual.Players)
                    //game.KillPlayer(otherPlayer.PlayerIndex);
            }
            for (auto& player : individual.Players)
            {
                for (uint32_t neuronIndex = 0; neuronIndex < player.Network.Neurons.size(); neuronIndex++)
                {
                    if (!player.Network.Neurons[neuronIndex].PendingTrigger) continue;
                    player.Network.Neurons[neuronIndex].PendingTrigger = false;

                    const int32_t outputIndex = neuronIndex - INPUT_NEURONS - HIDDEN_NEURONS;
                    if (outputIndex >= 0 && outputIndex < OUTPUT_NEURONS)
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
            DrawSidebar(renderer.Renderer, gameState.Generation, individuals, game, history, gameState.Sigma, lastFrameTime);

            lastFrameTime = FrameEnd(renderer, frame, gameState.SpeedUp);
        }
    }
}

/*
static void StartSim(const Renderer& renderer, FlappyBird& game)
{
    // State
    NeuralNetwork network;
    SetupNetwork(network);

    serialib serial;
    serial.openDevice("COM4", 9600);

    std::string rxBuffer;

    std::array<ScopeBuffer, SCOPE_COUNT> scopes;
    std::array<uint32_t, SCOPE_COUNT> scopeNeuronIndices = { 1 };

    bool quit = false;
    bool pause = false;

    uint32_t birdIndex = game.AddBird();

    SpikeEncoder spikeEncoder(10.0f, 0.0f);

    double lastFrameTime = 0.0;

    Uint64 lastGameUpdate = 0;

    while (!quit)
    {
        Frame frame = FrameStart(renderer);

        bool jump = HandleInputs(quit, pause);

        bool jumpNeuron = false; // SerialReadToBuffer(serial, rxBuffer);

        if (!pause)
        {
            game.Step(SIM_DT);
            if (jump || jumpNeuron)
            {
                game.BirdJump(birdIndex);
            }
            if (game.IsDone())
            {
                game.Reset();
                birdIndex = game.AddBird();
            }

            float gapDist = game.BirdGapDist(birdIndex);
            spikeEncoder.SetValue(-gapDist, -270.0f, 270.0f);
            bool spike = spikeEncoder.Update(SIM_DT);
            if (spike)
            {
                serial.writeString("!\n");
            }
        }

        game.Render();
        Drawer::DrawSidebar(renderer.Renderer, lastFrameTime, spikeEncoder.GetPhase());

        lastFrameTime = FrameEnd(renderer, frame);
    }

    serial.closeDevice();
}
*/

int main(int argc, char* argv[])
{

    const Renderer renderer = StartSDL();
    std::mt19937 rng(std::random_device{}());
    CartPole game(renderer.Renderer, rng);

    //StartSim(renderer, game);
    StartTraining(renderer, game, rng);

    StopSDL(renderer);
    
    return 0;
}