#pragma once

#include "Game.h"
#include "Drawer.h"
#include "Neuron.h"

class CartPole : public Game
{
public:
    CartPole(SDL_Renderer* renderer, std::mt19937& rng, uint32_t gameHeight = Drawer::DEFAULT_WINDOW_HEIGHT, uint32_t gameWidth = Drawer::DEFAULT_WINDOW_WIDTH);

    uint32_t AddPlayer(bool display) override;

    void Action(uint32_t playerIndex, uint32_t outputIndex) override;
    float GetInput(uint32_t playerIndex, uint32_t inputIndex) const override;

    bool PlayerAlive(uint32_t playerIndex) const override { return m_Players[playerIndex].Alive; }
    double PlayerFitness(uint32_t playerIndex) const override { return m_Players[playerIndex].Fitness; }
    uint32_t PlayerCount() const override { return static_cast<uint32_t>(m_Players.size()); }
    uint32_t AliveCount() const override { return m_AliveCount; }

    void Step(float dt) override;
    void Render() override;

    bool IsDone() const override { return m_Done; }
    double GetSimTime() const override { return m_SimTime; }

    void Reset() override;
    void KillPlayer(uint32_t playerIndex) override;
private:
    static constexpr uint32_t PHYS_STEPS = 1;

    static constexpr double CART_MASS = 1.0;
    static constexpr double POLE_MASS = 0.1;
    static constexpr double POLE_HALF_LENGTH = 0.5;
    static constexpr double GRAVITY_ACCEL = 9.81;
    static constexpr double FORCE_MAGNITUDE = 6.0;
    static constexpr double ANGLE_LIMIT = 0.4;

    static constexpr float ANGLE_NORM = static_cast<float>(ANGLE_LIMIT);
    static constexpr float ANGULAR_VEL_NORM = 4.0f;
    static constexpr float POSITION_NORM = 2.4f;
    static constexpr float CART_VEL_NORM = 4.0f;

    struct PhysicsState
    {
        double X = 0.0;
        double XDot = 0.0;
        double Theta = 0.0;
        double ThetaDot = 0.0;
    };

    struct Player
    {
        PhysicsState State;
        double Fitness = 0.0;
        float PendingForce = 0.0f;
        bool Alive = true;
        bool Display = true;
    };

    PhysicsState StepPhysics(const PhysicsState& state, double force, double dt) const;
    bool IsTerminal(const PhysicsState& state) const;

    const Player* FindBestPlayer() const;
    float WorldToScreenX(double worldX, double cameraX) const;
    float GetTrackY() const;

    void DrawBackground() const;
    void DrawTrack(double cameraX) const;
    void DrawPlayer(const Player& player, bool isBest, double cameraX) const;

    SDL_Renderer* m_Renderer;
    std::mt19937& m_Rng;

    uint32_t m_GameHeight;
    uint32_t m_GameWidth;

    std::vector<Player> m_Players;
    std::vector<Player*> m_AlivePlayers;
    uint32_t m_AliveCount = 0;
    bool m_Done = false;
    double m_SimTime = 0.0;
};
