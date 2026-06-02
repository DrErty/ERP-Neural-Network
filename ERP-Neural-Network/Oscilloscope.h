#pragma once

#include "ERP.h"
#include "Drawer.h"

class Oscilloscope
{
public:
    Oscilloscope(uint32_t windowWidth = Drawer::DEFAULT_WINDOW_WIDTH, uint32_t windowHeight = Drawer::DEFAULT_WINDOW_HEIGHT);

    void SetWindowSize(uint32_t width, uint32_t height);

    void SetPosition(float centerX, float centerY);
    void SetSize(float width, float height);

    void SetRange(float minValue, float maxValue);
    void SetAutoScale(bool enabled) { m_AutoScale = enabled; }
    void SetTitle(std::string_view title) { m_Title = title; }

    void SetTraceColor(Drawer::Col c) { m_TraceColor = c; }
    void SetBackgroundColor(Drawer::Col c) { m_BgColor = c; }
    void SetBorderColor(Drawer::Col c) { m_BorderColor = c; }

    void Draw(SDL_Renderer* renderer, std::span<const Scalar> samples);
private:
    uint32_t m_WindowWidth;
    uint32_t m_WindowHeight;

    float m_SizeX = 0.5f;
    float m_SizeY = 0.3f;
    float m_CenterX = 1.0 - m_SizeX / 2.0f;
    float m_CenterY = 1.0 - m_SizeY / 2.0f;

    bool  m_AutoScale = true;
    float m_RangeMin = -1.0f;
    float m_RangeMax = 1.0f;

    std::string m_Title;

    Drawer::Col m_BgColor = { 12,  16,  28, 220 };
    Drawer::Col m_BorderColor = { 90, 110, 150, 255 };
    Drawer::Col m_GridColor = { 40,  50,  75, 120 };
    Drawer::Col m_ZeroColor = { 90, 110, 150, 180 };
    Drawer::Col m_TraceColor = { 80, 230, 140, 255 };
    Drawer::Col m_LabelColor = { 200, 215, 240, 230 };
};