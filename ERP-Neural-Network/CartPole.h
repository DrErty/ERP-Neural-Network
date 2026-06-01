#pragma once

#include "Drawer.h"
#include "Neuron.h"

class CartPole
{
public:
    CartPole(SDL_Renderer* renderer, uint32_t gameHeight = Drawer::DEFAULT_WINDOW_HEIGHT, uint32_t gameWidth = Drawer::DEFAULT_WINDOW_WIDTH);

    uint32_t AddPlayer(bool display, std::mt19937& rng);

    void Action(uint32_t playerIndex, double spikeFrequency);
    float GetInput(uint32_t playerIndex, uint32_t inputIndex) const;

    bool PlayerAlive(uint32_t playerIndex) const { return m_Players[playerIndex].Alive; }
    double PlayerFitness(uint32_t playerIndex) const { return m_Players[playerIndex].Fitness; }
    uint32_t PlayerCount() const { return static_cast<uint32_t>(m_Players.size()); }
    uint32_t AliveCount() const { return m_AliveCount; }

    void Step(float dt, bool strictMode);
    void Render();

    bool IsDone() const { return m_Done; }
    double GetSimTime() const { return m_SimTime; }

    void Reset();
    void KillPlayer(uint32_t playerIndex);
private:
    static constexpr uint32_t PHYS_STEPS = 4;

    static constexpr double CART_MASS = 1.0;
    static constexpr double POLE_MASS = 0.001;
    static constexpr double POLE_HALF_LENGTH = 0.5;
    static constexpr double GRAVITY_ACCEL = 9.81;
    static constexpr double FORCE_MAGNITUDE = 6.0;
    static constexpr double ANGLE_LIMIT = g_PI / 2.0;

    static constexpr float ANGLE_NORM = static_cast<float>(g_PI) / 4.0f;
    static constexpr float ANGULAR_VEL_NORM = 4.0f;
    static constexpr float POSITION_NORM = 4.0f;
    static constexpr float REWARD_RADIUS = 1.0f;
    static constexpr float CART_VEL_NORM = 16.0f;

    static constexpr double HELD_UP_ANGLE = g_PI / 2.0;

    static constexpr double KILL_TIME = 10.0;

    static constexpr bool MOVING_CAMERA = false;

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
        bool HeldUp = false;
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

    uint32_t m_GameHeight;
    uint32_t m_GameWidth;

    std::vector<Player> m_Players;
    std::vector<Player*> m_AlivePlayers;
    uint32_t m_AliveCount = 0;
    bool m_Done = false;
    double m_SimTime = 0.0;

    double m_CameraX = 0.0;
    double m_CameraSpeed = 0.0;
};
