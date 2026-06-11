#include "SimulationUI.h"

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

            Rect band = row.track;
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
            CommitEdit();
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
    Drawer::RenderLine(renderer, w, 0.0f, w, h);

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

        const std::string txt = editing ? (m_EditBuffer + "_") : FormatValue(s, s.Get());
        Drawer::DrawTextSlow(renderer, txt, row.box.X + row.box.W * 0.5f, row.box.Y + 5.0f, valueCol, Drawer::g_FontSmall, true);
    }
}

Dashboard::Dashboard(uint32_t windowWidth, uint32_t windowHeight)
    : m_WindowWidth(windowWidth)
    , m_WindowHeight(windowHeight)
    , m_Scope(windowWidth, windowHeight)
{
    m_Scope.SetTitle("Theta versus Time");
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
    Drawer::DrawTextSlow(renderer, "Neural Network", left + 8.0f, top + 6.0f, Drawer::Col{ 200, 215, 240, 230 }, Drawer::g_FontSmall, false);

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

    auto signedColor = [](Scalar value, uint8_t baseAlpha) -> Drawer::Col
        {
            const float t = static_cast<float>(std::clamp(value / WEIGHT_LIMIT, Scalar(-1.0), Scalar(1.0)));
            const float m = std::fabs(t);
            const Drawer::Col neutral{ 120, 128, 150, baseAlpha };
            const Drawer::Col positive{ 235, 140, 70, 255 };
            const Drawer::Col negative{ 70, 150, 235, 255 };
            Drawer::Col c = (t >= 0.0f) ? Drawer::LerpCol(neutral, positive, m)
                : Drawer::LerpCol(neutral, negative, m);
            c.a = static_cast<uint8_t>(static_cast<float>(baseAlpha) + (255.0f - static_cast<float>(baseAlpha)) * m);
            return c;
        };

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
                if (wv != Scalar(0.0))
                {
                    Drawer::SetColor(renderer, signedColor(wv, 24));
                    const P a = pos[layerStart[l - 1] + i];
                    const P b = pos[layerStart[l] + j];
                    Drawer::RenderLine(renderer, a.x, a.y, b.x, b.y);

                    const P m = { (a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f };

                    {
                        StaticBuffer<char> stringBuffer(1024);
                        StringBuilder stringBuilder(stringBuffer);
                        stringBuilder.Concat(wv);
                        stringBuilder.Concat(", ");
                        stringBuilder.Concat(wOff + j * prev + i);
                        stringBuilder.Compile();

                        Drawer::DrawTextSlow(renderer, stringBuffer.GetData(), m.x, m.y, Drawer::Col{ 200, 215, 240, 230 }, Drawer::g_FontSmall, false);
                    }
                }
            }
        }
        wOff += prev * cur;
    }

    uint32_t bOff = 0;
    for (uint32_t l = 0; l < LAYER_COUNT; ++l)
    {
        for (uint32_t k = 0; k < LAYER_SIZES[l]; ++k)
        {
            const P p = pos[layerStart[l] + k];
            const Drawer::Col face = (l == 0)
                ? Drawer::Col{ 150, 158, 178, 255 }
            : signedColor(genome.Biases[bOff + k], 90);
            Drawer::SetColor(renderer, Drawer::Col{ 20, 24, 36, 255 });
            Drawer::DrawCircle(renderer, p.x, p.y, radius + 1.5f);
            Drawer::SetColor(renderer, face);
            Drawer::DrawCircle(renderer, p.x, p.y, radius);
        }
        if (l >= 1)
            bOff += LAYER_SIZES[l];
    }
}
