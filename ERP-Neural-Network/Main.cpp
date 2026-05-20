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
#include <cassert>

#include <SDL3_ttf/SDL_ttf.h>
#include "serialib.h"

static constexpr double V_DD = 3; // Volt;
static constexpr double V_THRESHOLD = V_DD / 2.0; // Volt
static constexpr double V_REFRAC_START = V_DD / 4.0; // Volt
static constexpr double REFRAC_TIME = 15 / 1000.0; // Seconds

static constexpr double DISPLAY_DT = 1.0 / 60.0; // Seconds
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

static TTF_Font* g_FontSmall = nullptr;
static TTF_Font* g_FontMedium = nullptr;

struct Span
{
    double Value;
    double Min;
    double Max;

    Span(double value, double min, double max)
        : Value(value), Min(max), Max(max)
    {
        assert(min < max);
        assert(min < max);
    }

    double GetRange() const
    {
        return Max - Min;
    }
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

// This function written by Claude
static void DrawText(SDL_Renderer* rend, TTF_Font* font, std::string_view text, float x, float y, SDL_Color color, bool centered = false)
{
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.data(), text.length(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, surf);
    const float w = (float)surf->w;
    const float h = (float)surf->h;

    SDL_DestroySurface(surf);
    if (!tex)
        return;

    SDL_FRect dst = { centered ? x - w * 0.5f : x, centered ? y - h * 0.5f : y, w, h };

    SDL_RenderTexture(rend, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

// This function written by Claude
static SDL_FPoint NeuronCenter(uint32_t idx, float cx, float cy)
{
    SDL_FPoint p = { cx, cy };
    if (idx < RING_NEURONS)
    {
        // Ring neurons: evenly spaced on a circle, starting at the top
        const double angle = -M_PI / 2.0 + (2.0 * M_PI * idx) / RING_NEURONS;
        p.x = cx + (float)(RING_RADIUS * std::cos(angle));
        p.y = cy + (float)(RING_RADIUS * std::sin(angle));
    }
    else if (idx < RING_NEURONS + CROSS_NEURONS)
    {
        // Cross neurons (O-1, O-2, O-3): three points further out
        const uint32_t local = idx - RING_NEURONS;
        // Start O-1 at the top, then 120 degrees apart
        const double angle = -M_PI / 2.0 + (2.0 * M_PI * local) / CROSS_NEURONS;
        p.x = cx + (float)(CROSS_RADIUS * std::cos(angle));
        p.y = cy + (float)(CROSS_RADIUS * std::sin(angle));
    }
    else
    {
        // Turn neuron: fixed position outside the cluster
        p.x = cx + TURN_OFFSET_X;
        p.y = cy + TURN_OFFSET_Y;
    }
    return p;
}

// This function written by Claude
static void DrawScope(SDL_Renderer* rend, const ScopeBuffer& buf, float x, float y, float w, float h, uint32_t neuronIndex)
{
    // ---- Background panel ----
    SDL_FRect bg = { x, y, w, h };
    SDL_SetRenderDrawColor(rend, 20, 22, 28, 235);
    SDL_RenderFillRect(rend, &bg);

    // ---- Border ----
    SDL_SetRenderDrawColor(rend, 90, 95, 110, 255);
    SDL_RenderRect(rend, &bg);

    // ---- Threshold line (horizontal, at the very top of the plot area) ----
    // We map V_mem in [0, V_THRESHOLD] to plot y in [bottom, top].
    SDL_SetRenderDrawColor(rend, 200, 80, 80, 180);
    SDL_RenderLine(rend, x + 1.0f, y + 1.0f, x + w - 1.0f, y + 1.0f);

    // ---- Zero line (horizontal, at the bottom of the plot area) ----
    SDL_SetRenderDrawColor(rend, 60, 100, 60, 140);
    SDL_RenderLine(rend, x + 1.0f, y + h - 1.0f, x + w - 1.0f, y + h - 1.0f);

    // ---- Mid-V_threshold gridline (optional, helpful reference) ----
    SDL_SetRenderDrawColor(rend, 50, 55, 65, 255);
    SDL_RenderLine(rend, x + 1.0f, y + h * 0.5f, x + w - 1.0f, y + h * 0.5f);

    // ---- Trace ----
    // Each sample is one SIM_DT in time. The buffer holds up to SCOPE_BUFFER_SIZE samples;
    // oldest sample is plotted on the left edge, newest on the right edge.
    if (buf.Count >= 2)
    {
        SDL_SetRenderDrawColor(rend, 120, 200, 255, 255);

        // Walk through valid samples in chronological order.
        // Oldest sample lives at (Head - Count + SCOPE_BUFFER_SIZE) mod SCOPE_BUFFER_SIZE
        const size_t startIdx = (buf.Head + SCOPE_BUFFER_SIZE - buf.Count) % SCOPE_BUFFER_SIZE;
        const float xStep = w / (float)(SCOPE_BUFFER_SIZE - 1);
        // Left-pad the trace so a partially-filled buffer doesn't stretch:
        // newest sample always sits at the right edge.
        const float xOffset = w - xStep * (float)(buf.Count - 1);

        auto sampleAt = [&](size_t k) -> double
            {
                return buf.Samples[(startIdx + k) % SCOPE_BUFFER_SIZE];
            };

        auto vToY = [&](double v) -> float
            {
                // Clamp to [0, V_THRESHOLD] for display
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
            SDL_RenderLine(rend, prevX, prevY, curX, curY);
            prevX = curX;
            prevY = curY;
        }
    }

    // ---- Title in top-left of the scope panel ----
    std::array<char, 64> title = {};
    std::snprintf(title.data(), sizeof(char) * title.size(), "Neuron %u", neuronIndex);
    SDL_Color titleColor = SDL_Color{ 120, 200, 255, 255 };
    DrawText(rend, g_FontMedium, title.data(), x + 8.0f, y + 4.0f, titleColor);

    // ---- Axis labels for threshold and zero ----
    std::array<char, 16> vt = {};
    std::snprintf(vt.data(), sizeof(char) * vt.size(), "%.2f V", V_THRESHOLD);
    SDL_Color axisColor = { 160, 160, 170, 255 };
    DrawText(rend, g_FontSmall, vt.data(), x + w - 48.0f, y + 1.0f, axisColor);
    DrawText(rend, g_FontSmall, "0 V", x + w - 48.0f, y + h - 14.0f, axisColor);

    const double scopeDuration = (double)SCOPE_BUFFER_SIZE * SIM_DT;
    constexpr int NUM_TICKS = 7;
    constexpr float TICK_LEN = 5.0f;

    SDL_Color tickColor = { 160, 160, 170, 255 };
    SDL_SetRenderDrawColor(rend, 160, 160, 170, 255);

    for (int t = 0; t < NUM_TICKS; ++t)
    {
        const float frac = (float)t / (float)(NUM_TICKS - 1);
        const float tickX = x + 1.0f + frac * (w - 2.0f);

        // Tick mark sticking up from the bottom edge
        SDL_RenderLine(rend, tickX, y + h - 1.0f, tickX, y + h - 1.0f - TICK_LEN);

        // Faint vertical gridline up the plot for readability
        SDL_SetRenderDrawColor(rend, 45, 48, 56, 255);
        SDL_RenderLine(rend, tickX, y + 1.0f, tickX, y + h - 2.0f);
        SDL_SetRenderDrawColor(rend, 160, 160, 170, 255);

        std::array<char, 16> tickLabel = {};
        const double tickTime = frac * scopeDuration;
        std::snprintf(tickLabel.data(), sizeof(char) * tickLabel.size(), "%.0fms", tickTime * 1000.0);
        DrawText(rend, g_FontSmall, tickLabel.data(), tickX, y + h + 5.0f, tickColor, true);
    }
}

// This function written by Claude
// Helper: draw a thin line between two points (SDL_RenderLine handles this natively)
static void DrawConnection(SDL_Renderer* rend, SDL_FPoint from, SDL_FPoint to, double weight)
{
    if (weight < 0.0)
    {
        SDL_SetRenderDrawColor(rend, 120, 120, 230, 200); // blue, like the figure
    }
    else
    {
        SDL_SetRenderDrawColor(rend, 230, 120, 120, 200); // soft red, like the figure
    }
    SDL_RenderLine(rend, from.x, from.y, to.x, to.y);

    SDL_Color labelColor = { 160, 160, 170, 255 };
    std::array<char, 16> label = {};
    std::snprintf(label.data(), sizeof(char) * label.size(), "%.1fV", weight);
    DrawText(rend, g_FontSmall, label.data(), (from.x + to.x) / 2.0f, (from.y + to.y) / 2.0f, labelColor, true);

    // Small arrowhead at the destination so direction is visible
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;
    const float ux = dx / len;
    const float uy = dy / len;
    const float arrowLen = 20.0f;
    const float arrowWid = 10.0f;
    // Place the tip just before the target so it doesn't disappear under the square
    const float tipX = to.x - ux * (NEURON_SIZE * 0.5f);
    const float tipY = to.y - uy * (NEURON_SIZE * 0.5f);
    const float baseX = tipX - ux * arrowLen;
    const float baseY = tipY - uy * arrowLen;
    // Perpendicular
    const float px = -uy;
    const float py = ux;
    SDL_RenderLine(rend, tipX, tipY, baseX + px * arrowWid, baseY + py * arrowWid);
    SDL_RenderLine(rend, tipX, tipY, baseX - px * arrowWid, baseY - py * arrowWid);
}

// This function written by Claude
static void RenderNetwork(SDL_Renderer* rend, NeuralNetwork& network, const std::array<ScopeBuffer, SCOPE_COUNT>& scopes, const std::array<uint32_t, SCOPE_COUNT>& scopeNeuronIndices)
{
    // Clear background to a light color
    SDL_SetRenderDrawColor(rend, 245, 245, 248, 255);
    SDL_RenderClear(rend);

    // Window center (assumes 1280x720 from CreateWindow; query if you resize)
    // TODO
    int winW = 1280, winH = 720;
    SDL_GetCurrentRenderOutputSize(rend, &winW, &winH);
    const float cx = winW * 0.5f;
    const float cy = winH * 0.5f;

    // ---- Draw the blue circle behind the ring (decorative, like the figure) ----
    SDL_SetRenderDrawColor(rend, 200, 215, 240, 255);
    // SDL3 has no native filled circle; approximate with a fan of triangles.
    const uint32_t circleSegments = 64;
    const float circleRadius = CROSS_RADIUS + NEURON_SIZE * 0.7f;
    std::vector<SDL_Vertex> verts;
    verts.reserve(circleSegments + 2);
    SDL_FColor blueFill = { 200.0f / 255.0f, 215.0f / 255.0f, 240.0f / 255.0f, 1.0f };
    SDL_Vertex centerVert{};
    centerVert.position = { cx, cy };
    centerVert.color = blueFill;
    verts.push_back(centerVert);
    for (int i = 0; i <= circleSegments; ++i)
    {
        const double a = (2.0 * M_PI * i) / circleSegments;
        SDL_Vertex v{};
        v.position = { cx + (float)(circleRadius * std::cos(a)),
                       cy + (float)(circleRadius * std::sin(a)) };
        v.color = blueFill;
        verts.push_back(v);
    }
    std::vector<int> indices;
    indices.reserve(circleSegments * 3);
    for (int i = 1; i <= circleSegments; ++i)
    {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    SDL_RenderGeometry(rend, nullptr, verts.data(), (int)verts.size(), indices.data(), (int)indices.size());

    // ---- Draw connections first (so they appear behind the neuron squares) ----
    // Each neuron's OutputConnections[k] stores the index of a neuron whose
    // axonal output feeds synapse k of THIS neuron. So the arrow goes from
    // that other neuron's center TO this neuron's center.
    for (uint32_t i = 0; i < TOTAL_NEURONS; ++i)
    {
        const SDL_FPoint from = NeuronCenter(i, cx, cy);
        for (uint32_t k = 0; k < MAX_OUTPUTS; ++k)
        {
            const int8_t src = network.Neurons[i].OutputConnections[k];
            if (src < 0 || src >= TOTAL_NEURONS) continue;
            const SDL_FPoint to = NeuronCenter((uint32_t)src, cx, cy);

            int32_t inputSyn = -1;
            for (uint32_t j = 0; j < MAX_INPUTS; j++)
            {
                if (network.Neurons[src].InputConnections[j] == i)
                    inputSyn = j;
            }
            if (inputSyn == -1)
            {
                SDL_Log("Very bad!");
                continue;
            }

            DrawConnection(rend, from, to, network.Neurons[src].Weights[inputSyn]);
        }
    }

    // ---- Draw each neuron as a square with a V_mem meter beneath it ----
    for (uint32_t i = 0; i < TOTAL_NEURONS; ++i)
    {
        const SDL_FPoint center = NeuronCenter(i, cx, cy);

        // Neuron body (dark purple square, like the figure)
        SDL_FRect body = {
            center.x - NEURON_SIZE * 0.5f,
            center.y - NEURON_SIZE * 0.5f,
            NEURON_SIZE,
            NEURON_SIZE
        };
        SDL_SetRenderDrawColor(rend, 60, 35, 75, 255);
        SDL_RenderFillRect(rend, &body);

        // Meter outline below the body
        SDL_FRect meterOutline = {
            center.x - METER_WIDTH * 0.5f,
            center.y + NEURON_SIZE * 0.5f + METER_GAP,
            METER_WIDTH,
            METER_HEIGHT
        };
        SDL_SetRenderDrawColor(rend, 80, 80, 90, 255);
        SDL_RenderRect(rend, &meterOutline);

        // Meter fill proportional to V_mem / V_THRESHOLD (clamped to [0,1])
        const double v = network.Neurons[i].V_mem;
        double frac = v / V_THRESHOLD;
        if (frac < 0.0) frac = 0.0;
        if (frac > 1.0) frac = 1.0;
        SDL_FRect meterFill = {
            meterOutline.x + 1.0f,
            meterOutline.y + 1.0f,
            (float)((METER_WIDTH - 2.0f) * frac),
            METER_HEIGHT - 2.0f
        };
        // Color shifts from green (low) toward red (near threshold)
        const Uint8 r = (Uint8)(60 + 195 * frac);
        const Uint8 g = (Uint8)(200 - 150 * frac);
        SDL_SetRenderDrawColor(rend, r, g, 60, 255);
        SDL_RenderFillRect(rend, &meterFill);

        // ---- Label below the meter ----
        std::array<char, 16> label = {};
        if (i < RING_NEURONS)
            std::snprintf(label.data(), sizeof(char) * label.size(), "R%u", i);
        else if (i < RING_NEURONS + CROSS_NEURONS)
            std::snprintf(label.data(), sizeof(char) * label.size(), "O-%u", i - RING_NEURONS + 1);
        else
            std::snprintf(label.data(), sizeof(char) * label.size(), "Turn");

        SDL_Color labelColor = { 40, 40, 50, 255 };
        DrawText(rend, g_FontSmall, label.data(), center.x, meterOutline.y + METER_HEIGHT + 4.0f, labelColor, true);
    }

    const float scopeX = winW - SCOPE_WIDTH - SCOPE_MARGIN;
    for (uint32_t i = 0; i < SCOPE_COUNT; i++)
    {
        const float scopeY = SCOPE_MARGIN + (SCOPE_HEIGHT + 20.0f) * i;
        DrawScope(rend, scopes[i], scopeX, scopeY, SCOPE_WIDTH, SCOPE_HEIGHT, scopeNeuronIndices[i]);
    }
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

static int StartSim()
{
    // Game State
    NeuralNetwork network;
    SetupNetwork(network);

    serialib serial;
    serial.openDevice("COM3", 9600);

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
    SDL_Window* win = SDL_CreateWindow("Evolutionary Training", 1280, 720, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* rend = SDL_CreateRenderer(win, nullptr);
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    if (!TTF_Init())
    {
        SDL_Log("TTF_Init: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    g_FontSmall = TTF_OpenFont("font.ttf", 12.0f);
    g_FontMedium = TTF_OpenFont("font.ttf", 16.0f);
    if (!g_FontSmall || !g_FontMedium)
    {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
    }

    Uint64 frame_start = SDL_GetPerformanceCounter();
    while (!quit)
    {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                quit = true;
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE)
                    quit = true;
                if (event.key.key == SDLK_SPACE)
                    pause = !pause;
                if (event.key.key == SDLK_R)
                {
                    serial.writeString("Input\n");
                }
            }
        }

        if (!pause)
        {
            double timeLeft = DISPLAY_DT;
            while (timeLeft > 0.0)
            {
                UpdateNetwork(network, scopes, scopeNeuronIndices);
                timeLeft -= SIM_DT;
            }
        }

        while (serial.available() > 0)
        {
            char c;
            serial.readChar(&c, 0);
            if (c != '\r')
            {
                rxBuffer += c;
            }

            if (c == '\n')
            {
                std::cout << rxBuffer;
                rxBuffer.clear();
            }
        }

        RenderNetwork(rend, network, scopes, scopeNeuronIndices);

        SDL_RenderPresent(rend);

        {
            // Frame pacing
            Uint64 currentTime = SDL_GetPerformanceCounter();
            double elapsedTime = static_cast<double>(currentTime - frame_start) / static_cast<double>(SDL_GetPerformanceFrequency());
            if (elapsedTime < DISPLAY_DT)
                SDL_Delay(static_cast<Uint32>((DISPLAY_DT - elapsedTime) * 1000.0));

            frame_start = SDL_GetPerformanceCounter();
        }
    }

    if (g_FontSmall)
        TTF_CloseFont(g_FontSmall);

    if (g_FontMedium)
        TTF_CloseFont(g_FontMedium);

    TTF_Quit();

    SDL_DestroyRenderer(rend);
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