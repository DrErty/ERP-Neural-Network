#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "Drawer.h"

class FlappyBird
{
public:
    static constexpr float GRAVITY = 2100.0f;
    static constexpr float FLAP_VEL = -630.0f;
    static constexpr float PIPE_SPEED = 330.0f;
    static constexpr float PIPE_GAP = 270.0f;
    static constexpr float PIPE_W = 90.0f;
    static constexpr float PIPE_SPACING = 480.0f;
    static constexpr float BIRD_X = 240.0f;
    static constexpr float BIRD_R = 21.0f;
    static constexpr uint32_t N_PIPES = 4;
    static constexpr uint32_t PHYS_STEPS = 16;

    FlappyBird(SDL_Renderer* renderer,
        TTF_Font* fontLarge,
        TTF_Font* fontSmall,
        std::mt19937& rng,
        uint32_t winW = Drawer::DEFAULT_WINDOW_WIDTH,
        uint32_t winH = Drawer::DEFAULT_WINDOW_HEIGHT);

    uint32_t AddBird(std::optional<Drawer::Col> color = std::nullopt, void* userData = nullptr);

    void BirdJump(uint32_t index);

    float BirdY(uint32_t index) const { return m_Birds[index].Y; }
    float BirdVy(uint32_t index) const { return m_Birds[index].Vy; }
    bool BirdAlive(uint32_t index) const { return m_Birds[index].Alive; }
    double BirdFitness(uint32_t index) const { return m_Birds[index].Fitness; }
    uint32_t BirdPipes(uint32_t index) const { return m_Birds[index].PipesCleared; }
    void* BirdUserData(uint32_t index) const { return m_Birds[index].UserData; }
    Drawer::Col BirdColor(uint32_t index) const { return m_Birds[index].Color; }
    float BirdGapDist(uint32_t index) const;

    uint32_t BirdCount() const { return (uint32_t)m_Birds.size(); }
    uint32_t AliveCount() const { return m_AliveCount; }

    bool HasNearestPipe() const;
    float NearestPipeGapY() const;
    float NearestPipeX() const;
    float NearestPipeDist() const;

    void Step(float dt = 1.0f / 60.0f);
    void Render();
    void PresentFrame() { SDL_RenderPresent(m_Renderer); }

    bool IsDone() const { return m_Done; }
    double SimTime() const { return m_SimTime; }
    double BestLiveFitness() const;

    void Reset();

    uint32_t WinW() const { return m_WinW; }
    uint32_t WinH() const { return m_WinH; }
    uint32_t GameW() const { return m_GameW; }
    float GroundY() const { return m_GroundY; }

private:
    struct Pipe
    {
        float X, GapY;
        bool Scored = false;
        float TopBot() const { return GapY - PIPE_GAP * 0.5f; }
        float BottomTop() const { return GapY + PIPE_GAP * 0.5f; }
    };

    struct Bird
    {
        float Y = 0.0f;
        float Vy = 0.0f;
        bool Alive = true;
        double Fitness = 0.0;
        uint32_t PipesCleared = 0;
        Drawer::Col Color = { 100, 255, 150, 220 };
        void* UserData = nullptr;
    };

    bool Collides(const Bird& bird) const;
    void SpawnPipes();
    void AdvancePipes(float dt);
    Pipe* NearestPipe();
    const Pipe* NearestPipeConst() const;
    Drawer::Col RandomColor();

    void DrawSky();
    void DrawGround();
    void DrawPipes();
    void DrawBirds();

    SDL_Renderer* m_Renderer;
    TTF_Font* m_FontLarge;
    TTF_Font* m_FontSmall;
    std::mt19937& m_Rng;

    uint32_t m_WinW, m_WinH, m_GameW;
    float m_GroundY;

    std::vector<Bird> m_Birds;
    std::vector<Pipe> m_Pipes;

    uint32_t m_AliveCount = 0;
    bool m_Done = false;
    double m_SimTime = 0.0;
};