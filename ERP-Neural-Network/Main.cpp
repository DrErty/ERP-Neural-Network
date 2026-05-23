#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

#include <SDL3_ttf/SDL_ttf.h>
#include "serialib.h"

#include "Drawer.h"
#include "FlappyBird.h"

static constexpr double V_DD = 3; // Volt;
static constexpr double V_THRESHOLD = V_DD / 2.0 * (1.3 / 1.5); // Volt
static constexpr double V_REFRAC_START = V_DD / 4.0; // Volt
static constexpr double REFRAC_TIME = 15 / 1000.0; // Seconds

static constexpr double DISPLAY_DT = 1.0 / 120.0; // Seconds
static constexpr double SIM_DT = 1 / 200.0; // Seconds

static constexpr uint32_t RING_NEURONS = 6;
static constexpr uint32_t TURN_NEURONS = 1;
static constexpr uint32_t CROSS_NEURONS = 3;
static constexpr uint32_t TOTAL_NEURONS = RING_NEURONS + CROSS_NEURONS + TURN_NEURONS;

static constexpr uint32_t MAX_INPUTS = 3;
static constexpr uint32_t MAX_OUTPUTS = 4;

static constexpr float M_PI = 3.14159265f;
static constexpr float NEURON_SIZE = 80.0f;         // px, square side length
static constexpr float METER_WIDTH = NEURON_SIZE;   // px, meter same width as neuron
static constexpr float METER_HEIGHT = 16.0f;         // px, height of V_mem bar
static constexpr float METER_GAP = 8.0f;            // px, gap between neuron and meter
static constexpr float RING_RADIUS = 400.0f;        // px, radius of inner ring
static constexpr float CROSS_RADIUS = 200.0f;       // px, radius for O-1, O-2, O-3
static constexpr float TURN_OFFSET_X = -500.0f;     // px, offset of turn neuron from center
static constexpr float TURN_OFFSET_Y = -400.0f;

static constexpr uint32_t SCOPE_COUNT = 3;
static constexpr size_t SCOPE_BUFFER_SIZE = 200;
static constexpr float SCOPE_WIDTH = 380.0f;
static constexpr float SCOPE_HEIGHT = 120.0f;
static constexpr float SCOPE_MARGIN = 20.0f;

class SpikeEncoder
{
public:
    SpikeEncoder(float maxRateHz = 300.0f, float minRateHz = 0.0f)
        : m_MaxRate(maxRateHz)
        , m_MinRate(minRateHz)
        , m_CurrentRate(0.0f)
        , m_Phase(0.0f)
    {}

    void SetValue(float value, float valueMin, float valueMax)
    {
        const float normalised = std::clamp((value - valueMin) / (valueMax - valueMin), 0.0f, 1.0f);
        m_CurrentRate = m_MinRate + normalised * (m_MaxRate - m_MinRate);
    }

    bool Update(float dt)
    {
        if (m_CurrentRate <= 0.0f) return false;
        m_Phase += m_CurrentRate * dt;
        if (m_Phase >= 1.0f)
        {
            m_Phase -= 1.0f;
            return true;
        }
        return false;
    }

    void Reset() { m_Phase = 0.0f; }
    float CurrentRate() const { return m_CurrentRate; }
    float GetPhase() const { return m_Phase; }
private:
    float m_MaxRate;
    float m_MinRate;
    float m_CurrentRate;
    float m_Phase;
};

struct Neuron
{
    std::array<double, MAX_INPUTS> Weights = {}; // Volt
    std::array<double, MAX_INPUTS> I_in = { 0.0, 0.0, 0.0 }; // 0 to 1

    double V_mem = 0.0; // Volt
    double Tau_mem = 0.3; // Seconds
    double Tau_syn = 0.07; // Seconds
    double V_leak = 0.7; // Volt

    double RefracTime = 0.0;

    std::array<int8_t, MAX_OUTPUTS> OutputConnections = { -1, -1, -1, -1 };
    std::array<int8_t, MAX_INPUTS> InputConnections = { -1, -1, -1 };
};

struct NeuralNetwork
{
    std::array<Neuron, TOTAL_NEURONS> Neurons = {};
};

struct ScopeBuffer
{
    std::array<double, SCOPE_BUFFER_SIZE> Samples = {};
    size_t Head = 0;
    size_t Count = 0;
};

// Written by Claude
static void ScopePush(ScopeBuffer& buf, double sample)
{
    buf.Samples[buf.Head] = sample;
    buf.Head = (buf.Head + 1) % SCOPE_BUFFER_SIZE;
    if (buf.Count < SCOPE_BUFFER_SIZE)
        buf.Count++;
}

static bool ConnectNeurons(NeuralNetwork& network, int8_t inputNeuron, int8_t outputNeuron, double weight)
{
    int32_t outputSyn = -1;
    for (uint32_t i = 0; i < MAX_OUTPUTS; i++)
    {
        if (network.Neurons[outputNeuron].OutputConnections[i] == -1)
            outputSyn = i;
    }

    int32_t inputSyn = -1;
    for (uint32_t i = 0; i < MAX_INPUTS; i++)
    {
        if (network.Neurons[inputNeuron].InputConnections[i] == -1)
            inputSyn = i;
    }

    if ((outputSyn == -1) || (inputSyn == -1))
    {
        std::array<char, 128> title = {};
        std::snprintf(title.data(), sizeof(char) * title.size(), "No connection possilbe %i %i %i %i", inputNeuron, outputNeuron, inputSyn, outputSyn);
        SDL_Log(title.data());
        return false;
    }

    network.Neurons[outputNeuron].OutputConnections[outputSyn] = inputNeuron;
    network.Neurons[inputNeuron].InputConnections[inputSyn] = outputNeuron;
    network.Neurons[inputNeuron].Weights[inputSyn] = weight;

    //std::printf("Connected neuron from %i to %i at synapse %i and %i\n", outputNeuron, inputNeuron, outputSyn, inputSyn);

    return true;
}

static void UpdateNetwork(NeuralNetwork& network, std::array<ScopeBuffer, SCOPE_COUNT>& scopes, const std::array<uint32_t, SCOPE_COUNT>& scopeNeuronIndices)
{
    const double dt = SIM_DT;
    for (uint32_t i = 0; i < TOTAL_NEURONS; i++)
    {
        Neuron& neuron = network.Neurons[i];
        double I_syn = 0.0;
        for (uint32_t j = 0; j < MAX_INPUTS; j++)
        {
            neuron.I_in[j] += (-neuron.I_in[j] / neuron.Tau_syn) * dt; // Expontential Decay
            I_syn += neuron.Weights[j] * neuron.I_in[j];
        }

        neuron.RefracTime -= dt;
        if (neuron.RefracTime <= 0.0)
        {
            const double dV = (-(neuron.V_mem - neuron.V_leak) + I_syn) / neuron.Tau_mem;
            neuron.V_mem += dV * dt;
        }

        // Check if should trigger
        if (neuron.V_mem >= V_THRESHOLD)
        {
            for (uint32_t j = 0; j < MAX_OUTPUTS; j++)
            {
                int8_t toIndex = neuron.OutputConnections[j];
                if (toIndex == -1) continue;

                int32_t inputSyn = -1;
                for (uint32_t k = 0; k < MAX_INPUTS; k++)
                {
                    if (network.Neurons[toIndex].InputConnections[k] == i)
                        inputSyn = k;
                }
                if (inputSyn == -1)
                {
                    SDL_Log("Very bad!");
                    continue;
                }

                network.Neurons[toIndex].I_in[inputSyn] = 1.0;
            }
            neuron.V_mem = 0;
            neuron.RefracTime = REFRAC_TIME;
        }
    }
    for (uint32_t i = 0; i < SCOPE_COUNT; i++)
    {
        ScopePush(scopes[i], network.Neurons[scopeNeuronIndices[i]].V_mem);
    }
}

static void DrawScope(SDL_Renderer* renderer, const ScopeBuffer& buf, float x, float y, float w, float h, uint32_t neuronIndex)
{
    SDL_FRect bg = { x, y, w, h };
    SDL_SetRenderDrawColor(renderer, 20, 22, 28, 235);
    SDL_RenderFillRect(renderer, &bg);

    SDL_SetRenderDrawColor(renderer, 90, 95, 110, 255);
    SDL_RenderRect(renderer, &bg);

    SDL_SetRenderDrawColor(renderer, 200, 80, 80, 180);
    SDL_RenderLine(renderer, x + 1.0f, y + 1.0f, x + w - 1.0f, y + 1.0f);

    SDL_SetRenderDrawColor(renderer, 60, 100, 60, 140);
    SDL_RenderLine(renderer, x + 1.0f, y + h - 1.0f, x + w - 1.0f, y + h - 1.0f);

    SDL_SetRenderDrawColor(renderer, 50, 55, 65, 255);
    SDL_RenderLine(renderer, x + 1.0f, y + h * 0.5f, x + w - 1.0f, y + h * 0.5f);

    if (buf.Count >= 2)
    {
        SDL_SetRenderDrawColor(renderer, 120, 200, 255, 255);

        const size_t startIdx = (buf.Head + SCOPE_BUFFER_SIZE - buf.Count) % SCOPE_BUFFER_SIZE;
        const float xStep = w / (float)(SCOPE_BUFFER_SIZE - 1);

        const float xOffset = w - xStep * (float)(buf.Count - 1);

        auto sampleAt = [&](size_t k) -> double
            {
                return buf.Samples[(startIdx + k) % SCOPE_BUFFER_SIZE];
            };

        auto vToY = [&](double v) -> float
            {
                double frac = v / V_THRESHOLD;
                if (frac < 0.0) frac = 0.0;
                if (frac > 1.0) frac = 1.0;
                return y + h - 1.0f - (float)(frac * (h - 2.0f));
            };

        float prevX = x + xOffset;
        float prevY = vToY(sampleAt(0));
        for (size_t k = 1; k < buf.Count; ++k)
        {
            const float curX = x + xOffset + xStep * (float)k;
            const float curY = vToY(sampleAt(k));
            SDL_RenderLine(renderer, prevX, prevY, curX, curY);
            prevX = curX;
            prevY = curY;
        }
    }

    std::array<char, 64> title = {};
    std::snprintf(title.data(), sizeof(char) * title.size(), "Neuron %u", neuronIndex);
    Drawer::Col titleColor = { 120, 200, 255, 255 };
    Drawer::DrawTextSlow(renderer, title.data(), x + 8.0f, y + 4.0f, titleColor, Drawer::g_FontMedium);

    std::array<char, 16> vt = {};
    std::snprintf(vt.data(), sizeof(char) * vt.size(), "%.2f V", V_THRESHOLD);
    Drawer::Col axisColor = { 160, 160, 170, 255 };
    Drawer::DrawTextSlow(renderer, vt.data(), x + w - 48.0f, y + 1.0f, axisColor, Drawer::g_FontSmall);
    Drawer::DrawTextSlow(renderer, "0 V", x + w - 48.0f, y + h - 14.0f, axisColor, Drawer::g_FontSmall);

    const double scopeDuration = (double)SCOPE_BUFFER_SIZE * SIM_DT;
    constexpr int NUM_TICKS = 7;
    constexpr float TICK_LEN = 5.0f;

    Drawer::Col tickColor = { 160, 160, 170, 255 };
    SDL_SetRenderDrawColor(renderer, 160, 160, 170, 255);

    for (int t = 0; t < NUM_TICKS; ++t)
    {
        const float frac = (float)t / (float)(NUM_TICKS - 1);
        const float tickX = x + 1.0f + frac * (w - 2.0f);

        SDL_RenderLine(renderer, tickX, y + h - 1.0f, tickX, y + h - 1.0f - TICK_LEN);

        SDL_SetRenderDrawColor(renderer, 45, 48, 56, 255);
        SDL_RenderLine(renderer, tickX, y + 1.0f, tickX, y + h - 2.0f);
        SDL_SetRenderDrawColor(renderer, 160, 160, 170, 255);

        std::array<char, 16> tickLabel = {};
        const double tickTime = frac * scopeDuration;
        std::snprintf(tickLabel.data(), sizeof(char) * tickLabel.size(), "%.0fms", tickTime * 1000.0);
        Drawer::DrawTextSlow(renderer, tickLabel.data(), tickX, y + h + 5.0f, tickColor, Drawer::g_FontSmall, true);
    }
}

static void DrawConnection(SDL_Renderer* renderer, SDL_FPoint from, SDL_FPoint to, double weight)
{
    if (weight < 0.0)
        SDL_SetRenderDrawColor(renderer , 120, 120, 230, 200);
    else
        SDL_SetRenderDrawColor(renderer , 230, 120, 120, 200);

    SDL_RenderLine(renderer , from.x, from.y, to.x, to.y);

    Drawer::Col labelColor = { 160, 160, 170, 255 };
    std::array<char, 16> label = {};
    std::snprintf(label.data(), sizeof(char) * label.size(), "%.1fV", weight);
    Drawer::DrawTextSlow(renderer, label.data(), (from.x + to.x) / 2.0f, (from.y + to.y) / 2.0f, labelColor, Drawer::g_FontSmall, true);

    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;
    const float ux = dx / len;
    const float uy = dy / len;
    const float arrowLen = 20.0f;
    const float arrowWid = 10.0f;

    const float tipX = to.x - ux * (NEURON_SIZE * 0.5f);
    const float tipY = to.y - uy * (NEURON_SIZE * 0.5f);
    const float baseX = tipX - ux * arrowLen;
    const float baseY = tipY - uy * arrowLen;

    const float px = -uy;
    const float py = ux;

    SDL_RenderLine(renderer , tipX, tipY, baseX + px * arrowWid, baseY + py * arrowWid);
    SDL_RenderLine(renderer , tipX, tipY, baseX - px * arrowWid, baseY - py * arrowWid);
}

static void SetupNetwork(NeuralNetwork& network)
{
    network.Neurons[TOTAL_NEURONS - 1].V_leak = 2.0;
    network.Neurons[TOTAL_NEURONS - 1].Tau_mem = 10.0;

    ConnectNeurons(network, 0, TOTAL_NEURONS - 1, 2.5);
    ConnectNeurons(network, 2, TOTAL_NEURONS - 1, 2.5);
    ConnectNeurons(network, 4, TOTAL_NEURONS - 1, 2.5);

    ConnectNeurons(network, 1, RING_NEURONS + 0, -1.2);
    ConnectNeurons(network, 3, RING_NEURONS + 1, -1.2);
    ConnectNeurons(network, 5, RING_NEURONS + 2, -1.2);

    ConnectNeurons(network, RING_NEURONS + 0, 3, 2.5);
    ConnectNeurons(network, RING_NEURONS + 0, 5, 2.5);

    ConnectNeurons(network, RING_NEURONS + 1, 1, 2.5);
    ConnectNeurons(network, RING_NEURONS + 1, 5, 2.5);

    ConnectNeurons(network, RING_NEURONS + 2, 1, 2.5);
    ConnectNeurons(network, RING_NEURONS + 2, 3, 2.5);

    for (uint32_t i = 0; i < RING_NEURONS; i++)
    {
        ConnectNeurons(network, (i + 1) % RING_NEURONS, i, 2.5);
    }

    ConnectNeurons(network, 1, 1, 2.5);
    ConnectNeurons(network, 3, 3, 2.5);
    ConnectNeurons(network, 5, 5, 2.5);

    network.Neurons[1].V_mem = V_THRESHOLD * 2.0;

    network.Neurons[1].Tau_syn = 0.5;
    network.Neurons[3].Tau_syn = 0.5;
    network.Neurons[5].Tau_syn = 0.5;
}

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

static bool HandleInputs(bool& quit, bool& pause, serialib& serial)
{
    bool jump = false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            quit = true;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
                quit = true;
            if (event.key.key == SDLK_R)
                pause = !pause;
            if (event.key.key == SDLK_SPACE)
            {
                jump = true;
            }
        }
    }
    return jump;
}

static int StartSim()
{
    // Game State
    NeuralNetwork network;
    SetupNetwork(network);

    serialib serial;
    serial.openDevice("COM4", 9600);

    std::string rxBuffer;

    std::array<ScopeBuffer, SCOPE_COUNT> scopes;
    std::array<uint32_t, SCOPE_COUNT> scopeNeuronIndices = { 1, 3, 5 };

    bool quit = false;
    bool pause = false;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Evolutionary Training", Drawer::DEFAULT_WINDOW_WIDTH, Drawer::DEFAULT_WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(win, nullptr);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    StartSDLTTF();

    std::mt19937 rng(std::random_device{}());
    FlappyBird game(renderer, Drawer::g_FontMedium, Drawer::g_FontSmall, rng, Drawer::DEFAULT_WINDOW_WIDTH, Drawer::DEFAULT_WINDOW_HEIGHT);
    uint32_t birdIndex = game.AddBird();

    SpikeEncoder spikeEncoder(10.0f, 0.0f);

    Uint64 frame_start = SDL_GetPerformanceCounter();
    double frameTime = 0.0;

    while (!quit)
    {

        bool jump = HandleInputs(quit, pause, serial);

        bool jumpNeuron = SerialReadToBuffer(serial, rxBuffer);

        if (!pause)
        {
            game.Step(frameTime);
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
            bool spike = spikeEncoder.Update(frameTime);
            if (spike)
            {
                serial.writeString("!\n");
            }
        }

        game.Render();
        Drawer::DrawSidebar(renderer, frameTime, spikeEncoder.GetPhase());

        SDL_RenderPresent(renderer);

        {
            Uint64 currentTime = SDL_GetPerformanceCounter();
            double elapsedTime = static_cast<double>(currentTime - frame_start) / static_cast<double>(SDL_GetPerformanceFrequency());
            //if (elapsedTime < DISPLAY_DT)
            //    SDL_Delay(static_cast<Uint32>((DISPLAY_DT - elapsedTime) * 1000.0));

            frameTime = elapsedTime;

            frame_start = SDL_GetPerformanceCounter();
        }
    }

    StopSDLTTF();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    serial.closeDevice();

    return 0;
}

int main(int argc, char* argv[])
{
    //return 0;
    return StartSim();
}