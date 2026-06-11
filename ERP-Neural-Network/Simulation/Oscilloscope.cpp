#include "Oscilloscope.h"

Oscilloscope::Oscilloscope(uint32_t windowWidth, uint32_t windowHeight)
    : m_WindowWidth(windowWidth)
    , m_WindowHeight(windowHeight)
{
}

void Oscilloscope::SetWindowSize(uint32_t width, uint32_t height)
{
    m_WindowWidth = width;
    m_WindowHeight = height;
}

void Oscilloscope::SetPosition(float centerX, float centerY)
{
    m_CenterX = centerX;
    m_CenterY = centerY;
}

void Oscilloscope::SetSize(float width, float height)
{
    m_SizeX = width;
    m_SizeY = height;
}

void Oscilloscope::SetRange(float minValue, float maxValue)
{
    m_RangeMin = minValue;
    m_RangeMax = maxValue;
    m_AutoScale = false;
}

void Oscilloscope::Draw(SDL_Renderer* renderer, std::span<const Scalar> samples)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const float W = static_cast<float>(m_WindowWidth);
    const float H = static_cast<float>(m_WindowHeight);

    auto ndcToPxX = [W](float nx) { return (nx * 0.5f + 0.5f) * W; };
    auto ndcToPxY = [H](float ny) { return (1.0f - (ny * 0.5f + 0.5f)) * H; };

    const float halfW = m_SizeX * 0.5f;
    const float halfH = m_SizeY * 0.5f;
    const float left = ndcToPxX(m_CenterX - halfW);
    const float right = ndcToPxX(m_CenterX + halfW);
    const float top = ndcToPxY(m_CenterY + halfH);
    const float bottom = ndcToPxY(m_CenterY - halfH);
    const float pxW = right - left;
    const float pxH = bottom - top;

    Drawer::SetColor(renderer, m_BgColor);
    Drawer::FillRect(renderer, left, top, pxW, pxH);

    Drawer::SetColor(renderer, m_GridColor);
    constexpr int divs = 4;
    for (int i = 1; i < divs; ++i)
    {
        const float gx = left + pxW * static_cast<float>(i) / divs;
        Drawer::RenderLine(renderer, gx, top, gx, bottom);
        const float gy = top + pxH * static_cast<float>(i) / divs;
        Drawer::RenderLine(renderer, left, gy, right, gy);
    }

    float lo = m_RangeMin;
    float hi = m_RangeMax;
    if (m_AutoScale && !samples.empty())
    {
        lo = hi = static_cast<float>(samples[0]);
        for (Scalar s : samples)
        {
            const float v = static_cast<float>(s);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
        const float pad = (hi - lo) * 0.1f;
        lo -= pad;
        hi += pad;
    }
    if (hi - lo < 1e-6f)
    {
        lo -= 0.5f;
        hi += 0.5f;
    }

    auto valueToPxY = [&](float v)
        {
            const float n = (v - lo) / (hi - lo);
            return bottom - n * pxH;
        };

    if (lo < 0.0f && hi > 0.0f)
    {
        Drawer::SetColor(renderer, m_ZeroColor);
        const float zy = valueToPxY(0.0f);
        Drawer::RenderLine(renderer, left, zy, right, zy);
    }

    Drawer::SetColor(renderer, m_TraceColor);
    const std::size_t n = samples.size();
    if (n == 1)
    {
        const float y = valueToPxY(static_cast<float>(samples[0]));
        Drawer::RenderLine(renderer, left, y, right, y);
    }
    else if (n >= 2)
    {
        float prevX = left;
        float prevY = valueToPxY(static_cast<float>(samples[0]));
        for (std::size_t i = 1; i < n; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(n - 1);
            const float x = left + t * pxW;
            const float y = valueToPxY(static_cast<float>(samples[i]));
            Drawer::RenderLine(renderer, prevX, prevY, x, y);
            prevX = x;
            prevY = y;
        }
    }

    Drawer::SetColor(renderer, m_BorderColor);
    Drawer::DrawRect(renderer, left, top, pxW, pxH);

    if (!m_Title.empty())
        Drawer::DrawTextSlow(renderer, m_Title, left + 6.0f, top + 4.0f, m_LabelColor, Drawer::g_FontSmall, false);

    if (!samples.empty())
    {
        StaticBuffer<char> stringBuffer(32);
        StringBuilder stringBuilder(stringBuffer);
        stringBuilder.Concat(static_cast<Scalar>(samples.back()));
        stringBuilder.Compile();

        Drawer::DrawTextSlow(renderer, stringBuffer.GetData(), right - 56.0f, top + 4.0f, m_LabelColor, Drawer::g_FontSmall, false);
    }
}
