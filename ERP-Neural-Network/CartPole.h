#pragma once

#include "Drawer.h"
#include "Neuron.h"

class CartPole
{
public:
    struct PhysicsState
    {
        Scalar X = Scalar(0.0);
        Scalar XDot = Scalar(0.0);
        Scalar Theta = Scalar(0.0);
        Scalar ThetaDot = Scalar(0.0);
    };

    CartPole(SDL_Renderer* renderer, uint32_t gameHeight = Drawer::DEFAULT_WINDOW_HEIGHT, uint32_t gameWidth = Drawer::DEFAULT_WINDOW_WIDTH);

    uint32_t AddPlayer(bool display, std::mt19937& rng);

    void SetForce(uint32_t playerIndex, Scalar strength);
    std::array<Scalar, 5> GetInputs(uint32_t playerIndex) const;

    const PhysicsState& GetState(uint32_t playerIndex) const;

    bool PlayerAlive(uint32_t playerIndex) const { return m_Players[playerIndex].Alive; }
    Scalar PlayerFitness(uint32_t playerIndex) const { return m_Players[playerIndex].Fitness; }
    uint32_t PlayerCount() const { return static_cast<uint32_t>(m_Players.size()); }
    uint32_t AliveCount() const { return m_AliveCount; }

    void Step(Scalar dt);
    void Render();

    bool IsDone() const { return m_Done; }
    double GetSimTime() const { return m_SimTime; }

    void Reset();
    void KillPlayer(uint32_t playerIndex);
private:
    static constexpr uint32_t PHYS_STEPS = 16;

    static constexpr Scalar CART_MASS = Scalar(1.0);
    static constexpr Scalar POLE_MASS = Scalar(0.1);
    static constexpr Scalar POLE_HALF_LENGTH = Scalar(0.5);
    static constexpr Scalar GRAVITY_ACCEL = Scalar(9.81);
    static constexpr Scalar FORCE_MAGNITUDE = Scalar(8.0);

    static constexpr Scalar ANGLE_NORM = g_PI;
    static constexpr Scalar ANGULAR_VEL_NORM = Scalar(4.0);
    static constexpr Scalar POSITION_NORM = Scalar(3.0);
    static constexpr Scalar CART_VEL_NORM = Scalar(16.0);

    static constexpr Scalar HELD_UP_ANGLE = g_PI / Scalar(6.0);
    static constexpr Scalar KILL_ANGLE = g_PI / Scalar(2.0);

    static constexpr Scalar KILL_TIME = Scalar(10.0);

    static constexpr bool MOVING_CAMERA = false;

    struct Player
    {
        PhysicsState State;
        bool HeldUp = false;
        Scalar Fitness = 0.0;
        Scalar PendingForce = 0.0f;
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

    uint32_t m_GameHeight;
    uint32_t m_GameWidth;

    std::vector<Player> m_Players;
    std::vector<Player*> m_AlivePlayers;
    uint32_t m_AliveCount = 0;
    bool m_Done = false;
    Scalar m_SimTime = 0.0;

    Scalar m_CameraX = 0.0;
    Scalar m_CameraSpeed = 0.0;
};
