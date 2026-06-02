#include "CartPole.h"

#include <algorithm>
#include <cmath>

CartPole::CartPole(SDL_Renderer* renderer, uint32_t gameHeight, uint32_t gameWidth)
    : m_Renderer(renderer)
    , m_GameHeight(gameHeight)
    , m_GameWidth(gameWidth)
{
}

uint32_t CartPole::AddPlayer(bool display, std::mt19937& rng)
{
    const uint32_t playerIndex = static_cast<uint32_t>(m_Players.size());
    Player player;
    std::uniform_real_distribution<double> distribution(-1.0, 1.0);
    player.State.Theta = g_PI / 4.0 * distribution(rng);
    player.Display = display;

    if (playerIndex == 0)
    {
        m_CameraX = 0.0;
        m_CameraSpeed = 0.0;
    }

    m_Players.push_back(std::move(player));
    ++m_AliveCount;
    return playerIndex;
}

void CartPole::Action(uint32_t playerIndex, double spikeFrequency)
{
    Assert(playerIndex < m_Players.size());

    if (m_SimTime < TIME_SETTING)
        m_BaseLineFrequency = spikeFrequency;

    m_BaseLineFrequency = 0.0;

    Player& player = m_Players[playerIndex];
    if (!player.Alive) return;

    if (std::abs(player.State.Theta) > g_PI / 2.0)
    {
        // Swing-up: abs inputs, sign computed externally
        double sign = (player.State.ThetaDot * std::cos(player.State.Theta) >= 0.0) ? -1.0 : 1.0;
        player.PendingForce = std::abs(spikeFrequency - m_BaseLineFrequency) * sign / MAX_OUTPUT_RATE_OFFSET * FORCE_MAGNITUDE;
    }
    else
    {
        const double sign = player.State.Theta > 0.0 ? 1.0 : -1.0;
        player.PendingForce = (spikeFrequency - m_BaseLineFrequency) * sign / MAX_OUTPUT_RATE_OFFSET * FORCE_MAGNITUDE;
    }
}

void CartPole::SetForce(uint32_t playerIndex, double strength)
{
    Assert(playerIndex < m_Players.size());

    Player& player = m_Players[playerIndex];
    if (!player.Alive) return;
    
    player.PendingForce = std::clamp(strength, -1.0, 1.0) * FORCE_MAGNITUDE;
}

double CartPole::GetInput(uint32_t playerIndex, uint32_t inputIndex) const
{
    Assert(playerIndex < m_Players.size());

    const PhysicsState& state = m_Players[playerIndex].State;

    const double sign = state.Theta > 0.0 ? 1.0 : -1.0;

    if (INPUT_NEURON_COUNT == 3)
    {
        if (m_SimTime < TIME_SETTING)
        {
            if (inputIndex == 0) return 0.0;
            if (inputIndex == 1) return 0.0;
            if (inputIndex == 2) return 0.0;
            //if (inputIndex == 3) return 0.0;
            //if (inputIndex == 4) return 0.0;
            //if (inputIndex == 5) return 1.0;
        }
        else
        {
            if (inputIndex == 0) return state.Theta / ANGLE_NORM * sign;
            if (inputIndex == 1) return state.ThetaDot / ANGULAR_VEL_NORM * sign;
            if (inputIndex == 2) return 0.0;
            //if (inputIndex == 3) return state.X / POSITION_NORM;
            //if (inputIndex == 4) return state.XDot / CART_VEL_NORM;
            //if (inputIndex == 5) return 1.0;
        }
    }
    else
    {
        Assert(false);
    }

    return 0.0f;
}

std::array<Scalar, 4> CartPole::GetInputs(uint32_t playerIndex) const
{
    Assert(playerIndex < m_Players.size());

    const PhysicsState& state = m_Players[playerIndex].State;

    const double thetaNormed = state.Theta / ANGLE_NORM;
    const double thetaDotNormed = state.ThetaDot / ANGULAR_VEL_NORM;
    const double XNormed = state.X / POSITION_NORM;
    const double XDotNormed = state.XDot / CART_VEL_NORM;
    return { thetaNormed, thetaDotNormed, XNormed, XDotNormed };
}

CartPole::PhysicsState CartPole::StepPhysics(const PhysicsState& state, double force, double dt) const
{
    const double sinTheta = std::sin(state.Theta);
    const double cosTheta = std::cos(state.Theta);
    const double denominator = CART_MASS + POLE_MASS * sinTheta * sinTheta;

    const double xDotDot = (force + POLE_MASS * POLE_HALF_LENGTH * state.ThetaDot * state.ThetaDot * sinTheta - POLE_MASS * GRAVITY_ACCEL * sinTheta * cosTheta) / denominator;
    const double thetaDotDot = ((CART_MASS + POLE_MASS) * GRAVITY_ACCEL * sinTheta - force * cosTheta - POLE_MASS * POLE_HALF_LENGTH * state.ThetaDot * state.ThetaDot * sinTheta * cosTheta) / (POLE_HALF_LENGTH * denominator);

    PhysicsState next;
    next.X = state.X + state.XDot * dt;
    next.XDot = state.XDot + xDotDot * dt;
    next.Theta = state.Theta + state.ThetaDot * dt;
    next.ThetaDot = state.ThetaDot + thetaDotDot * dt;

    if (next.Theta >= g_PI)
        next.Theta -= g_PI * 2.0;

    if (next.Theta <= -g_PI)
        next.Theta += g_PI * 2.0;

    return next;
}

bool CartPole::IsTerminal(const PhysicsState& state) const
{
    return std::abs(state.X) > static_cast<double>(POSITION_NORM);
}

void CartPole::Step(float dt, bool strictMode)
{
    if (m_Done) return;

    const double physDt = static_cast<double>(dt) / static_cast<double>(PHYS_STEPS);

    if (m_Players.size() > 0 and MOVING_CAMERA)
    {
        Player& lastBestPlayer = m_Players[0];
        for (uint32_t substep = 0; substep < PHYS_STEPS; ++substep)
        {
            m_CameraSpeed += (lastBestPlayer.State.XDot - m_CameraSpeed) * physDt * 1.0;

            m_CameraX += m_CameraSpeed * physDt;
            m_CameraX += (lastBestPlayer.State.X - m_CameraX) * physDt * 1.0;
        }
    }

    for (auto& player : m_Players)
    {
        if (!player.Alive) continue;
        m_AlivePlayers.emplace_back(&player);
    }

    std::for_each(std::execution::par, m_AlivePlayers.begin(), m_AlivePlayers.end(), [this, physDt, strictMode](Player* player)
        {
            for (uint32_t substep = 0; substep < PHYS_STEPS; ++substep)
            {
                const double appliedForce = static_cast<double>(player->PendingForce);

                if (m_SimTime > TIME_SETTING)
                    player->State = StepPhysics(player->State, appliedForce, physDt);

                const double angleFraction = 1.0 - std::abs(player->State.Theta) / ANGLE_LIMIT;
                const double angleDotFraction = 1.0 - std::abs(player->State.ThetaDot) / static_cast<double>(ANGULAR_VEL_NORM);
                //const double positionFraction = 1.0 - std::min(1.0, std::abs(player->State.X) / static_cast<double>(REWARD_RADIUS));
                const double velocityFraction = 1.0 - std::abs(player->State.XDot) / static_cast<double>(CART_VEL_NORM);

                double positionFraction = 0.0;
                if (std::abs(player->State.X) < REWARD_RADIUS)
                    positionFraction = 1.0;

                const double E_pole = 0.5 * POLE_MASS * POLE_HALF_LENGTH * POLE_HALF_LENGTH
                    * player->State.ThetaDot * player->State.ThetaDot
                    - POLE_MASS * GRAVITY_ACCEL * POLE_HALF_LENGTH
                    * std::cos(player->State.Theta);
                const double E_target = -POLE_MASS * GRAVITY_ACCEL * POLE_HALF_LENGTH;
                const double E_max = POLE_MASS * GRAVITY_ACCEL * POLE_HALF_LENGTH * 2.0;
                const double energyReward = 1.0 - std::min(1.0, std::abs(E_pole - E_target) / E_max);

                if (m_SimTime > TIME_SETTING)
                {
                    player->Fitness += energyReward * physDt * 200.0;
                    player->Fitness += physDt;
                    player->Fitness += std::max(0.0, std::abs(angleFraction) * angleFraction) * physDt * 200.0;
                    //player->Fitness -= 20.0 * appliedForce * appliedForce * physDt;
                    player->Fitness += std::max(0.0, positionFraction) * physDt * 10.0;
                    player->Fitness += std::max(0.0, velocityFraction) * physDt * 10.0;
                    player->Fitness += std::max(0.0, angleDotFraction) * physDt * 200.0;
                }
            }
        });

    m_AlivePlayers.clear();

    for (auto& player : m_Players)
    {
        if (player.State.Theta <= HELD_UP_ANGLE and player.State.Theta >= -HELD_UP_ANGLE)
            player.HeldUp = true;
        else
        {
            if (player.HeldUp)
            {
                player.Alive = false;
                --m_AliveCount;
            }
        }

        if ((m_SimTime > KILL_TIME) and (not player.HeldUp))
        {
            player.Alive = false;
            --m_AliveCount;
        }

        if (IsTerminal(player.State) and player.Alive)
        {
            //player.Alive = false;
            //--m_AliveCount;
        }
    }

    m_SimTime += dt;
    if (m_AliveCount == 0)
        m_Done = true;
}

void CartPole::Render()
{
    SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);
    DrawBackground();

    const Player* bestPlayer = FindBestPlayer();
    double cameraX = bestPlayer ? bestPlayer->State.X : 0.0;
    cameraX = m_CameraX;

    DrawTrack(cameraX);

    for (const Player& player : m_Players)
    {
        if (!player.Alive) continue;
        if (!player.Display) continue;
        DrawPlayer(player, &player == bestPlayer, cameraX);
    }

    if (bestPlayer && !bestPlayer->Alive)
        DrawPlayer(*bestPlayer, true, cameraX);
}

void CartPole::Reset()
{
    m_Players.clear();
    m_AliveCount = 0;
    m_Done = false;
    m_SimTime = 0.0;
}

void CartPole::KillPlayer(uint32_t playerIndex)
{
    Assert(playerIndex < m_Players.size());
    if (m_Players[playerIndex].Alive)
    {
        m_Players[playerIndex].Alive = false;
        --m_AliveCount;
    }
}

const CartPole::Player* CartPole::FindBestPlayer() const
{
    const Player* bestPlayer = nullptr;
    for (const auto& player : m_Players)
        if (player.Alive && (!bestPlayer || player.Fitness > bestPlayer->Fitness)) bestPlayer = &player;
    if (!bestPlayer)
        for (const auto& player : m_Players)
            if (!bestPlayer || player.Fitness > bestPlayer->Fitness) bestPlayer = &player;
    return bestPlayer;
}

float CartPole::WorldToScreenX(double worldX, double cameraX) const
{
    const float pixelsPerMeter = static_cast<float>(m_GameWidth) * 0.8f / (POSITION_NORM * 2.0f);
    const float screenCentreX = static_cast<float>(m_GameWidth) * 0.5f;
    return screenCentreX + static_cast<float>(worldX - cameraX) * pixelsPerMeter;
}

float CartPole::GetTrackY() const
{
    return static_cast<float>(m_GameHeight) * 0.65f;
}

void CartPole::DrawBackground() const
{
    for (uint32_t y = 0; y < m_GameHeight; ++y)
    {
        const float factor = static_cast<float>(y) / static_cast<float>(m_GameHeight);
        SDL_SetRenderDrawColor(m_Renderer, static_cast<uint8_t>(18.0f + factor * 8.0f), static_cast<uint8_t>(22.0f + factor * 12.0f), static_cast<uint8_t>(45.0f + factor * 18.0f), 255);
        SDL_RenderLine(m_Renderer, 0.0f, static_cast<float>(y), static_cast<float>(m_GameWidth), static_cast<float>(y));
    }
}

void CartPole::DrawTrack(double cameraX) const
{
    const float trackY = GetTrackY();
    const float trackThickness = 6.0f;

    Drawer::SetColor(m_Renderer, { 80, 90, 120, 255 });
    Drawer::FillRect(m_Renderer, 0.0f, trackY, static_cast<float>(m_GameWidth), trackThickness);

    const float originScreenX = WorldToScreenX(0.0, cameraX);
    Drawer::SetColor(m_Renderer, { 60, 70, 100, 120 });
    SDL_RenderLine(m_Renderer, originScreenX, trackY - 24.0f, originScreenX, trackY + trackThickness);

    const float referenceLeft = WorldToScreenX(-static_cast<double>(POSITION_NORM), cameraX);
    const float referenceRight = WorldToScreenX(static_cast<double>(POSITION_NORM), cameraX);
    Drawer::SetColor(m_Renderer, { 100, 80, 80, 80 });
    SDL_RenderLine(m_Renderer, referenceLeft, trackY - 16.0f, referenceLeft, trackY + trackThickness);
    SDL_RenderLine(m_Renderer, referenceRight, trackY - 16.0f, referenceRight, trackY + trackThickness);
}

void CartPole::DrawPlayer(const Player& player, bool isBest, double cameraX) const
{
    const float trackY = GetTrackY();
    const float pixelsPerMeter = static_cast<float>(m_GameWidth) * 0.8f / (POSITION_NORM * 2.0f);
    const float poleScreenLength = static_cast<float>(POLE_HALF_LENGTH * 2.0) * pixelsPerMeter;

    const float cartScreenX = WorldToScreenX(player.State.X, cameraX);
    const float cartWidth = 60.0f;
    const float cartHeight = 30.0f;
    const float cartTop = trackY - cartHeight;

    const Drawer::Col cartColor = isBest ? Drawer::Col{ 255, 220, 60, 220 } : Drawer::Col{ 100, 160, 255, 140 };
    Drawer::SetColor(m_Renderer, cartColor);
    Drawer::FillRect(m_Renderer, cartScreenX - cartWidth * 0.5f, cartTop, cartWidth, cartHeight);
    Drawer::SetColor(m_Renderer, { 200, 220, 255, 180 });
    Drawer::DrawRect(m_Renderer, cartScreenX - cartWidth * 0.5f, cartTop, cartWidth, cartHeight);

    const float wheelRadius = 10.0f;
    const float wheelY = trackY + 3.0f;
    Drawer::SetColor(m_Renderer, { 60, 70, 100, 255 });
    Drawer::DrawCircle(m_Renderer, cartScreenX - cartWidth * 0.3f, wheelY, wheelRadius);
    Drawer::DrawCircle(m_Renderer, cartScreenX + cartWidth * 0.3f, wheelY, wheelRadius);

    const float pivotX = cartScreenX;
    const float pivotY = cartTop;
    const float poleTipX = pivotX + static_cast<float>(std::sin(player.State.Theta)) * poleScreenLength;
    const float poleTipY = pivotY - static_cast<float>(std::cos(player.State.Theta)) * poleScreenLength;

    const float angleFraction = static_cast<float>(std::abs(player.State.Theta) / ANGLE_LIMIT);
    const Drawer::Col poleColor = isBest ? Drawer::LerpCol({ 60, 220, 120, 255 }, { 220, 60, 60, 255 }, angleFraction) : Drawer::Col{ 100, 180, 255, 120 };
    Drawer::SetColor(m_Renderer, poleColor);
    for (float offset = -2.0f; offset <= 2.0f; offset += 1.0f)
        SDL_RenderLine(m_Renderer, pivotX + offset, pivotY, poleTipX + offset, poleTipY);

    Drawer::SetColor(m_Renderer, { 255, 255, 255, 220 });
    Drawer::DrawCircle(m_Renderer, poleTipX, poleTipY, 6.0f);
    Drawer::SetColor(m_Renderer, { 60, 70, 100, 255 });
    Drawer::DrawCircle(m_Renderer, pivotX, pivotY, 5.0f);
}