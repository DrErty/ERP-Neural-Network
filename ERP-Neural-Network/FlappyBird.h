#pragma once

#include "ERP.h"

#include "Drawer.h"
#include "Game.h"
#include "Neuron.h"

class FlappyBird : public Game
{
public:
    FlappyBird(SDL_Renderer* renderer, std::mt19937& rng, uint32_t gameHeight = Drawer::DEFAULT_WINDOW_HEIGHT, uint32_t gameWidth = Drawer::DEFAULT_WINDOW_WIDTH);

    uint32_t AddPlayer(bool display) override;

    void Action(uint32_t playerIndex, uint32_t outputIndex) override;
    float GetInput(uint32_t playerIndex, uint32_t inputIndex) const override;

    bool PlayerAlive(uint32_t playerIndex) const override { return m_Birds[playerIndex].Alive; }
    double PlayerFitness(uint32_t playerIndex) const override { return m_Birds[playerIndex].Fitness; }

    uint32_t PlayerCount() const override { return (uint32_t)m_Birds.size(); }
    uint32_t AliveCount() const override { return m_AliveCount; }

    void Step(float dt) override;
    void Render() override;

    bool IsDone() const override { return m_Done; }
    double GetSimTime() const override { return m_SimTime; }

    void Reset() override;
    void KillPlayer(uint32_t playerIndex) override;
private:
    static constexpr float GRAVITY = 2100.0f;
    static constexpr float FLAP_VEL = -630.0f;
    static constexpr float PIPE_SPEED = 330.0f;
    static constexpr float PIPE_GAP = 270.0f;
    static constexpr float PIPE_W = 90.0f;
    static constexpr float PIPE_SPACING = 480.0f;
    static constexpr float BIRD_X = 240.0f;
    static constexpr float BIRD_R = 21.0f;
    static constexpr uint32_t N_PIPES = 4;
    static constexpr uint32_t PHYS_STEPS = 4;

    static constexpr uint32_t MAX_BIRD_DRAW_COUNT = 20;

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
    };

    bool Collides(const Bird& bird) const;
    void SpawnPipes();
    void AdvancePipes(float dt);
    Pipe* NearestPipe();
    const Pipe* NearestPipeConst() const;

    void DrawSky();
    void DrawGround();
    void DrawPipes();
    void DrawBirds();

    SDL_Renderer* m_Renderer;
    std::mt19937& m_Rng;

    uint32_t m_GameHeight, m_GameWidth;
    float m_GroundY;

    std::vector<Bird> m_Birds;
    std::vector<const Bird*> m_SortedBirds;
    std::vector<Pipe> m_Pipes;

    uint32_t m_AliveCount = 0;
    bool m_Done = false;
    double m_SimTime = 0.0;
};