#include "SimulationUI.h"

void DrawSidebar(SDL_Renderer* renderer, uint32_t generation, const std::vector<Individual>& individuals, CartPole& game, const std::vector<double>& history, double sigma, double frameTime)
{
    const double bestEver = history.size() > 0 ? *std::max_element(history.begin(), history.end()) : 0.0;

    const double bestAliveFitness = 0.0;

    const uint32_t m_GameW = Drawer::DEFAULT_WINDOW_WIDTH - Drawer::SIDEBAR_WIDTH;
    const uint32_t m_GameH = Drawer::DEFAULT_WINDOW_HEIGHT;

    const uint32_t m_WinW = Drawer::DEFAULT_WINDOW_WIDTH;
    const uint32_t m_WinH = Drawer::DEFAULT_WINDOW_HEIGHT;

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
    Drawer::DrawTextSlow(renderer, std::to_string(individualsAlive) + " / " + std::to_string(MAX_INDIVIDUALS), posX, posY, { 160, 200, 255, 220 }, Drawer::g_FontSmall);
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

        const uint32_t sampleCount = static_cast<uint32_t>(history.size());

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
    Drawer::DrawTextSlow(renderer, "[ESC] quit   [Space] Speed up   [R] Disable rendering", posX, (float)m_WinH - 26.0f,
        { 120, 130, 160, 200 }, Drawer::g_FontSmall);
    Drawer::DrawTextSlow(renderer, "Frame time: " + std::to_string(frameTime * 1000.0) + "ms", posX, (float)m_WinH - 52.0f,
        { 120, 130, 160, 200 }, Drawer::g_FontSmall);

    if (individuals.size() > 0)
    {
        const Player& player = individuals[0].Players2[0];
        const NeuralNetwork& baseNetwork = player.Network;

        constexpr uint32_t layerCount = 3;
        constexpr std::array<uint32_t, layerCount> layerSizes = { INPUT_NEURON_COUNT, HIDDEN_NEURON_COUNT, OUTPUT_NEURON_COUNT };
        constexpr std::array<uint32_t, layerCount> layerOffsets = { 0, INPUT_NEURON_COUNT, INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT };

        const float neuronRadius = 4.0f;
        const float networkHeight = 30.0f + 20.0f * TOTAL_NEURON_COUNT + 20.0f;

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

        std::array<float, TOTAL_NEURON_COUNT> neuronScreenX;
        std::array<float, TOTAL_NEURON_COUNT> neuronScreenY;
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

        for (uint32_t postNeuronIndex = 0; postNeuronIndex < TOTAL_NEURON_COUNT; ++postNeuronIndex)
        {
            const Neuron& postNeuron = baseNetwork.Neurons[postNeuronIndex];
            for (uint32_t inputSlot = 0; inputSlot < MAX_INPUTS; ++inputSlot)
            {
                const int8_t preNeuronIndex = postNeuron.InputConnections[inputSlot];
                if (preNeuronIndex < 0) continue;

                const double weight = baseNetwork.Weights[postNeuronIndex][inputSlot];
                const float weightNormalised = std::clamp(static_cast<float>(weight / 0.5), -1.0f, 1.0f);
                const float weightAbsolute = std::abs(weightNormalised);

                Drawer::Col lineColor;
                if (weightNormalised >= 0.0f)
                    lineColor = Drawer::LerpCol({ 100, 100, 100, 255 }, { 60, 220, 100, 255 }, weightAbsolute);
                else
                    lineColor = Drawer::LerpCol({ 100, 100, 100, 255 }, { 220, 60, 60, 255 }, weightAbsolute);

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

        for (uint32_t neuronIndex = 0; neuronIndex < TOTAL_NEURON_COUNT; ++neuronIndex)
        {
            const NeuronParams& params = baseNetwork.GetParams(neuronIndex);
            const float activation = static_cast<float>(std::clamp(baseNetwork.VMem[neuronIndex] / params.VThreshold, 0.0, 1.0));

            Drawer::Col baseColor;
            if (neuronIndex < INPUT_NEURON_COUNT)
                baseColor = { 100, 180, 255, 255 };
            else if (neuronIndex < INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT)
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

            if (neuronIndex >= INPUT_NEURON_COUNT + HIDDEN_NEURON_COUNT)
            {
                float frequency = baseNetwork.GetFrequency(neuronIndex - INPUT_NEURON_COUNT - HIDDEN_NEURON_COUNT);
                std::string freq = std::to_string(frequency);
                Drawer::DrawTextSlow(renderer, freq,
                    neuronScreenX[neuronIndex] - neuronRadius * 2,
                    neuronScreenY[neuronIndex],
                    { 255, 255, 255, 255 },
                    Drawer::g_FontSmall, true
                );
            }

            if (neuronIndex < INPUT_NEURON_COUNT)
            {
                float frequency = player.InputState[neuronIndex].CurrentRate();
                std::string freq = std::to_string(frequency);
                Drawer::DrawTextSlow(renderer, freq,
                    neuronScreenX[neuronIndex] - neuronRadius * 2,
                    neuronScreenY[neuronIndex],
                    { 255, 255, 255, 255 },
                    Drawer::g_FontSmall, true
                );
            }

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