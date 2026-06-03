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

SettingsPanel::SettingsPanel(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight)
    : m_Window(window)
    , m_WindowWidth(windowWidth)
    , m_WindowHeight(windowHeight)
{
}

void SettingsPanel::AddSlider(std::string label, float minValue, float maxValue,
    std::function<float()> get, std::function<void(float)> set, bool integer)
{
    m_Settings.push_back(Setting{ std::move(label), minValue, maxValue, integer, std::move(get), std::move(set) });
}

void SettingsPanel::AddSlider(std::string label, float minValue, float maxValue, float& value)
{
    AddSlider(std::move(label), minValue, maxValue,
        [&value] { return value; },
        [&value](float v) { value = v; },
        false);
}

void SettingsPanel::AddSlider(std::string label, float minValue, float maxValue, int& value)
{
    AddSlider(std::move(label), minValue, maxValue,
        [&value] { return static_cast<float>(value); },
        [&value](float v) { value = static_cast<int>(std::lround(v)); },
        true);
}

SettingsPanel::Row SettingsPanel::Layout(std::size_t index) const
{
    constexpr float pad = 16.0f;
    constexpr float titleH = 52.0f;
    constexpr float rowH = 70.0f;
    constexpr float boxW = 88.0f;
    constexpr float boxH = 26.0f;
    constexpr float trackH = 6.0f;

    const float innerW = static_cast<float>(Drawer::SIDEBAR_WIDTH) - 2.0f * pad;
    const float rowTop = titleH + static_cast<float>(index) * rowH;

    const Setting& s = m_Settings[index];
    const float v = s.Get();
    const float frac = (s.Max > s.Min) ? std::clamp((v - s.Min) / (s.Max - s.Min), 0.0f, 1.0f) : 0.0f;

    Row row;
    row.labelX = pad;
    row.labelY = rowTop + 14.0f;
    row.track.X = pad;
    row.track.Y = rowTop + 42.0f;
    row.track.W = innerW - boxW - 12.0f;
    row.track.H = trackH;
    row.handleX = row.track.X + frac * row.track.W;
    row.handleY = row.track.Y + row.track.H * 0.5f;
    row.box.X = pad + innerW - boxW;
    row.box.Y = rowTop + 34.0f;
    row.box.W = boxW;
    row.box.H = boxH;
    return row;
}

std::string SettingsPanel::FormatValue(const Setting& setting, float value) const
{
    if (setting.Integer)
        return std::format("{}", static_cast<int>(std::lround(value)));
    return std::format("{:.2f}", value);
}

void SettingsPanel::BeginEdit(std::size_t index)
{
    if (m_EditIndex >= 0 && static_cast<std::size_t>(m_EditIndex) != index)
        CommitEdit();

    m_EditIndex = static_cast<int>(index);
    m_EditBuffer = FormatValue(m_Settings[index], m_Settings[index].Get());
    m_EditFresh = true;
    if (m_Window)
        SDL_StartTextInput(m_Window);
}

void SettingsPanel::CommitEdit()
{
    if (m_EditIndex < 0)
        return;

    Setting& s = m_Settings[static_cast<std::size_t>(m_EditIndex)];

    float value = 0.0f;
    const char* begin = m_EditBuffer.data();
    const char* end = begin + m_EditBuffer.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc{} && ptr != begin)
    {
        value = std::clamp(value, s.Min, s.Max);
        if (s.Integer)
            value = std::round(value);
        s.Set(value);
    }

    m_EditIndex = -1;
    m_EditBuffer.clear();
    if (m_Window)
        SDL_StopTextInput(m_Window);
}

void SettingsPanel::CancelEdit()
{
    m_EditIndex = -1;
    m_EditBuffer.clear();
    if (m_Window)
        SDL_StopTextInput(m_Window);
}

void SettingsPanel::SetFromMouseX(std::size_t index, float mouseX)
{
    Setting& s = m_Settings[index];
    const Row row = Layout(index);
    float frac = (row.track.W > 0.0f) ? (mouseX - row.track.X) / row.track.W : 0.0f;
    frac = std::clamp(frac, 0.0f, 1.0f);
    float value = s.Min + frac * (s.Max - s.Min);
    if (s.Integer)
        value = std::round(value);
    s.Set(value);
}

void SettingsPanel::HandleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        if (event.button.button != SDL_BUTTON_LEFT)
            break;

        const float mx = event.button.x;
        const float my = event.button.y;

        bool hit = false;
        for (std::size_t i = 0; i < m_Settings.size(); ++i)
        {
            const Row row = Layout(i);

            if (row.box.Contains(mx, my))
            {
                BeginEdit(i);
                hit = true;
                break;
            }

            Rect band = row.track;          // generous grab band around the track
            band.Y -= 12.0f;
            band.H += 24.0f;
            if (band.Contains(mx, my))
            {
                if (m_EditIndex >= 0)
                    CommitEdit();
                m_DragIndex = static_cast<int>(i);
                SetFromMouseX(i, mx);
                hit = true;
                break;
            }
        }

        if (!hit && m_EditIndex >= 0)
            CommitEdit();                    // clicked empty space -> commit
        break;
    }

    case SDL_EVENT_MOUSE_MOTION:
        if (m_DragIndex >= 0)
            SetFromMouseX(static_cast<std::size_t>(m_DragIndex), event.motion.x);
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == SDL_BUTTON_LEFT)
            m_DragIndex = -1;
        break;

    case SDL_EVENT_TEXT_INPUT:
        if (m_EditIndex >= 0)
        {
            for (const char* p = event.text.text; *p != '\0'; ++p)
            {
                const char c = *p;
                const bool digit = (c >= '0' && c <= '9');
                const bool sign = (c == '-' && (m_EditFresh || m_EditBuffer.empty()));
                const bool dot = (c == '.' && (m_EditFresh || m_EditBuffer.find('.') == std::string::npos));
                if (digit || sign || dot)
                {
                    if (m_EditFresh)
                    {
                        m_EditBuffer.clear();
                        m_EditFresh = false;
                    }
                    m_EditBuffer.push_back(c);
                }
            }
        }
        break;

    case SDL_EVENT_KEY_DOWN:
        if (m_EditIndex >= 0)
        {
            const SDL_Keycode key = event.key.key;
            if (key == SDLK_BACKSPACE)
            {
                m_EditFresh = false;
                if (!m_EditBuffer.empty())
                    m_EditBuffer.pop_back();
            }
            else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
            {
                CommitEdit();
            }
            else if (key == SDLK_ESCAPE)
            {
                CancelEdit();
            }
        }
        break;

    default:
        break;
    }
}

void SettingsPanel::Draw(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const float w = static_cast<float>(Drawer::SIDEBAR_WIDTH);
    const float h = static_cast<float>(m_WindowHeight);

    Drawer::SetColor(renderer, Drawer::Col{ 18, 22, 34, 235 });
    Drawer::FillRect(renderer, 0.0f, 0.0f, w, h);
    Drawer::SetColor(renderer, Drawer::Col{ 70, 84, 120, 255 });
    SDL_RenderLine(renderer, w, 0.0f, w, h);

    if (Drawer::g_FontMedium)
        Drawer::DrawTextSlow(renderer, "Settings", 16.0f, 22.0f, Drawer::Col{ 220, 228, 245, 255 }, Drawer::g_FontMedium, false);

    const Drawer::Col labelCol{ 196, 206, 226, 255 };
    const Drawer::Col trackCol{ 44, 52, 74, 255 };
    const Drawer::Col fillCol{ 90, 170, 235, 255 };
    const Drawer::Col handleCol{ 220, 230, 248, 255 };
    const Drawer::Col boxCol{ 30, 36, 54, 255 };
    const Drawer::Col boxEdit{ 52, 66, 96, 255 };
    const Drawer::Col boxEdge{ 80, 92, 124, 255 };
    const Drawer::Col valueCol{ 224, 232, 248, 255 };

    for (std::size_t i = 0; i < m_Settings.size(); ++i)
    {
        const Setting& s = m_Settings[i];
        const Row row = Layout(i);

        if (Drawer::g_FontSmall)
            Drawer::DrawTextSlow(renderer, s.Label, row.labelX, row.labelY, labelCol, Drawer::g_FontSmall, false);

        Drawer::SetColor(renderer, trackCol);
        Drawer::FillRect(renderer, row.track.X, row.track.Y, row.track.W, row.track.H);
        Drawer::SetColor(renderer, fillCol);
        Drawer::FillRect(renderer, row.track.X, row.track.Y, row.handleX - row.track.X, row.track.H);
        Drawer::SetColor(renderer, handleCol);
        Drawer::DrawCircle(renderer, row.handleX, row.handleY, 9.0f);

        const bool editing = (m_EditIndex == static_cast<int>(i));
        Drawer::SetColor(renderer, editing ? boxEdit : boxCol);
        Drawer::FillRect(renderer, row.box.X, row.box.Y, row.box.W, row.box.H);
        Drawer::SetColor(renderer, boxEdge);
        Drawer::DrawRect(renderer, row.box.X, row.box.Y, row.box.W, row.box.H);

        if (Drawer::g_FontSmall)
        {
            const std::string txt = editing ? (m_EditBuffer + "_") : FormatValue(s, s.Get());
            Drawer::DrawTextSlow(renderer, txt, row.box.X + row.box.W * 0.5f, row.box.Y + 5.0f, valueCol, Drawer::g_FontSmall, true);
        }
    }
}

Dashboard::Dashboard(uint32_t windowWidth, uint32_t windowHeight)
    : m_WindowWidth(windowWidth)
    , m_WindowHeight(windowHeight)
    , m_Scope(windowWidth, windowHeight)
{
    m_Scope.SetTitle("output");
    m_Scope.SetRange(-1.0f, 1.0f);
}

void Dashboard::DrawScope(SDL_Renderer* renderer, std::span<const Scalar> samples)
{
    const float W = static_cast<float>(m_WindowWidth);
    const float H = static_cast<float>(m_WindowHeight);
    const float barLeft = W - static_cast<float>(Drawer::SIDEBAR_WIDTH);
    constexpr float pad = 18.0f;

    const float sx = barLeft + pad;
    const float sw = static_cast<float>(Drawer::SIDEBAR_WIDTH) - 2.0f * pad;
    const float sy = pad;
    const float sh = H * 0.30f;

    const float cx = sx + sw * 0.5f;
    const float cy = sy + sh * 0.5f;

    m_Scope.SetWindowSize(m_WindowWidth, m_WindowHeight);
    m_Scope.SetPosition(cx / W * 2.0f - 1.0f, 1.0f - cy / H * 2.0f);
    m_Scope.SetSize(sw / W * 2.0f, sh / H * 2.0f);
    m_Scope.Draw(renderer, samples);
}

void Dashboard::DrawNetwork(SDL_Renderer* renderer, const NetworkGenome& genome)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const float W = static_cast<float>(m_WindowWidth);
    const float H = static_cast<float>(m_WindowHeight);
    const float barLeft = W - static_cast<float>(Drawer::SIDEBAR_WIDTH);
    constexpr float pad = 18.0f;

    const float left = barLeft + pad;
    const float top = H * 0.36f;
    const float w = static_cast<float>(Drawer::SIDEBAR_WIDTH) - 2.0f * pad;
    const float h = H * 0.58f;

    Drawer::SetColor(renderer, Drawer::Col{ 12, 16, 28, 220 });
    Drawer::FillRect(renderer, left, top, w, h);
    Drawer::SetColor(renderer, Drawer::Col{ 70, 84, 120, 255 });
    Drawer::DrawRect(renderer, left, top, w, h);
    if (Drawer::g_FontSmall)
        Drawer::DrawTextSlow(renderer, "network", left + 8.0f, top + 6.0f, Drawer::Col{ 200, 215, 240, 230 }, Drawer::g_FontSmall, false);

    const float innerLeft = left + 22.0f;
    const float innerTop = top + 36.0f;
    const float innerW = w - 44.0f;
    const float innerH = h - 56.0f;

    struct P { float x, y; };
    std::vector<P> pos;
    std::array<uint32_t, LAYER_COUNT> layerStart{};
    uint32_t total = 0;
    uint32_t maxLayer = 1;
    for (uint32_t l = 0; l < LAYER_COUNT; ++l)
    {
        layerStart[l] = total;
        total += LAYER_SIZES[l];
        if (LAYER_SIZES[l] > maxLayer)
            maxLayer = LAYER_SIZES[l];
    }
    pos.reserve(total);
    for (uint32_t l = 0; l < LAYER_COUNT; ++l)
    {
        const float nx = (LAYER_COUNT == 1)
            ? innerLeft + innerW * 0.5f
            : innerLeft + innerW * static_cast<float>(l) / static_cast<float>(LAYER_COUNT - 1);
        const uint32_t count = LAYER_SIZES[l];
        for (uint32_t k = 0; k < count; ++k)
        {
            const float ny = innerTop + innerH * (static_cast<float>(k) + 0.5f) / static_cast<float>(count);
            pos.push_back(P{ nx, ny });
        }
    }

    const float radius = std::clamp(innerH / (static_cast<float>(maxLayer) * 2.4f), 3.0f, 11.0f);

    auto signedColor = [](float value, uint8_t baseAlpha) -> Drawer::Col
        {
            const float t = std::clamp(value / WEIGHT_LIMIT, -1.0, 1.0);
            const float m = std::fabs(t);
            const Drawer::Col neutral{ 120, 128, 150, baseAlpha };
            const Drawer::Col positive{ 235, 140, 70, 255 };
            const Drawer::Col negative{ 70, 150, 235, 255 };
            Drawer::Col c = (t >= 0.0f) ? Drawer::LerpCol(neutral, positive, m)
                : Drawer::LerpCol(neutral, negative, m);
            c.a = static_cast<uint8_t>(static_cast<float>(baseAlpha) + (255.0f - static_cast<float>(baseAlpha)) * m);
            return c;
        };

    // connections first (behind neurons), in the same layout the network uses
    uint32_t wOff = 0;
    for (uint32_t l = 1; l < LAYER_COUNT; ++l)
    {
        const uint32_t prev = LAYER_SIZES[l - 1];
        const uint32_t cur = LAYER_SIZES[l];
        for (uint32_t j = 0; j < cur; ++j)
        {
            for (uint32_t i = 0; i < prev; ++i)
            {
                const Scalar wv = genome.Weights[wOff + j * prev + i];
                if (wv != 0.0)
                {
                    Drawer::SetColor(renderer, signedColor(static_cast<float>(wv), 24));
                    const P a = pos[layerStart[l - 1] + i];
                    const P b = pos[layerStart[l] + j];
                    SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);

                    const P m = { (a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f };

                    if (Drawer::g_FontSmall)
                        Drawer::DrawTextSlow(renderer, std::to_string(wv) + ", " + std::to_string(wOff + j * prev + i), m.x, m.y, Drawer::Col{200, 215, 240, 230}, Drawer::g_FontSmall, false);
                }
            }
        }
        wOff += prev * cur;
    }

    // neurons (input layer neutral, others coloured by bias)
    uint32_t bOff = 0;
    for (uint32_t l = 0; l < LAYER_COUNT; ++l)
    {
        for (uint32_t k = 0; k < LAYER_SIZES[l]; ++k)
        {
            const P p = pos[layerStart[l] + k];
            const Drawer::Col face = (l == 0)
                ? Drawer::Col{ 150, 158, 178, 255 }
            : signedColor(static_cast<float>(genome.Biases[bOff + k]), 90);
            Drawer::SetColor(renderer, Drawer::Col{ 20, 24, 36, 255 });
            Drawer::DrawCircle(renderer, p.x, p.y, radius + 1.5f);
            Drawer::SetColor(renderer, face);
            Drawer::DrawCircle(renderer, p.x, p.y, radius);
        }
        if (l >= 1)
            bOff += LAYER_SIZES[l];
    }
}
