#pragma once

#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Evolution.h"
#include "Drawer.h"

#include "CartPole.h"
#include "ContinuousNetwork.h"

#include "Oscilloscope.h"

void DrawSidebar(SDL_Renderer* renderer, uint32_t generation, const std::vector<Individual>& individuals, CartPole& game, const std::vector<Scalar>& history, Scalar sigma, Scalar frameTime);

class SettingsPanel
{
public:
    SettingsPanel(SDL_Window* window,
        uint32_t windowWidth = Drawer::DEFAULT_WINDOW_WIDTH,
        uint32_t windowHeight = Drawer::DEFAULT_WINDOW_HEIGHT);

    void SetWindowSize(uint32_t width, uint32_t height) { m_WindowWidth = width; m_WindowHeight = height; }

    void AddSlider(std::string_view label, float minValue, float maxValue,
        std::function<float()> get, std::function<void(float)> set,
        bool integer = false);
    void AddSlider(std::string_view label, float minValue, float maxValue, float& value);
    void AddSlider(std::string_view label, float minValue, float maxValue, int& value);

    void AddToggle(std::string_view label, std::function<bool()> get, std::function<void(bool)> set);
    void AddToggle(std::string_view label, bool& value);

    void HandleEvent(const SDL_Event& event);
    void Draw(SDL_Renderer* renderer);

private:
    enum class Kind { Slider, Toggle };

    struct Setting
    {
        Kind                       Type = Kind::Slider;
        std::string                Label;
        float                      Min = 0.0f;
        float                      Max = 1.0f;
        bool                       Integer = false;
        std::function<float()>     Get;
        std::function<void(float)> Set;
        std::function<bool()>      GetBool;
        std::function<void(bool)>  SetBool;
    };

    struct Rect
    {
        float X = 0.0f, Y = 0.0f, W = 0.0f, H = 0.0f;
        bool Contains(float px, float py) const
        {
            return px >= X && px <= X + W && py >= Y && py <= Y + H;
        }
    };

    struct Row
    {
        float labelX = 0.0f, labelY = 0.0f;
        Rect  track;
        float handleX = 0.0f, handleY = 0.0f;
        Rect  box;
    };

    static float RowHeight(Kind kind);
    Row         Layout(std::size_t index) const;
    void        BeginEdit(std::size_t index);
    void        CommitEdit();
    void        CancelEdit();
    void        SetFromMouseX(std::size_t index, float mouseX);
    std::string FormatValue(const Setting& setting, float value) const;

    SDL_Window* m_Window;
    uint32_t    m_WindowWidth;
    uint32_t    m_WindowHeight;

    std::vector<Setting> m_Settings;

    int         m_DragIndex = -1;
    int         m_EditIndex = -1;
    bool        m_EditFresh = false;
    std::string m_EditBuffer;
};

class Dashboard
{
public:
    Dashboard(uint32_t windowWidth = Drawer::DEFAULT_WINDOW_WIDTH, uint32_t windowHeight = Drawer::DEFAULT_WINDOW_HEIGHT);

    void SetWindowSize(uint32_t width, uint32_t height) { m_WindowWidth = width; m_WindowHeight = height; }

    Oscilloscope& Scope() { return m_Scope; }

    void DrawScope(SDL_Renderer* renderer, std::span<const Scalar> samples);
    void DrawNetwork(SDL_Renderer* renderer, const NetworkGenome& genome);
private:
    uint32_t m_WindowWidth;
    uint32_t m_WindowHeight;
    Oscilloscope m_Scope;
};
