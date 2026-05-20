#include "FlappyBird.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

FlappyBird::FlappyBird(SDL_Renderer* renderer,
    TTF_Font* fontLarge,
    TTF_Font* fontSmall,
    std::mt19937& rng,
    uint32_t winW,
    uint32_t winH)
    : m_Renderer(renderer)
    , m_FontLarge(fontLarge)
    , m_FontSmall(fontSmall)
    , m_Rng(rng)
    , m_WinW(winW)
    , m_WinH(winH)
    , m_GroundY((float)winH - 90.0f)
    , m_GameW(winW)
{
    SpawnPipes();
}

uint32_t FlappyBird::AddBird(std::optional<Drawer::Col> color, void* userData)
{
    const uint32_t index = (uint32_t)m_Birds.size();
    Bird bird;
    bird.Y = (float)m_WinH * 0.45f;
    bird.Color = color.value_or(RandomColor());
    bird.UserData = userData;
    m_Birds.push_back(std::move(bird));
    ++m_AliveCount;
    return index;
}

void FlappyBird::BirdJump(uint32_t index)
{
    assert(index < m_Birds.size());
    if (m_Birds[index].Alive)
        m_Birds[index].Vy = FLAP_VEL;
}

float FlappyBird::BirdGapDist(uint32_t index) const
{
    const Pipe* const nearestPipe = NearestPipeConst();
    if (!nearestPipe) return 0.0f;
    return nearestPipe->GapY - m_Birds[index].Y;
}

bool FlappyBird::HasNearestPipe() const
{
    return NearestPipeConst() != nullptr;
}

float FlappyBird::NearestPipeGapY() const
{
    const Pipe* const nearestPipe = NearestPipeConst();
    return nearestPipe ? nearestPipe->GapY : (float)m_WinH * 0.5f;
}

float FlappyBird::NearestPipeX() const
{
    const Pipe* const nearestPipe = NearestPipeConst();
    return nearestPipe ? nearestPipe->X : (float)m_GameW + PIPE_SPACING;
}

float FlappyBird::NearestPipeDist() const
{
    return NearestPipeX() - BIRD_X;
}

void FlappyBird::Step(float dt)
{
    if (m_Done) return;

    const float physDt = dt / (float)PHYS_STEPS;

    for (uint32_t substep = 0; substep < PHYS_STEPS; ++substep)
    {
        AdvancePipes(physDt);
        Pipe* const nearestPipe = NearestPipe();

        for (auto& bird : m_Birds)
        {
            if (!bird.Alive) continue;

            bird.Vy += GRAVITY * physDt;
            bird.Vy = std::clamp(bird.Vy, -1050.0f, 1050.0f);
            bird.Y += bird.Vy * physDt;

            if (nearestPipe && !nearestPipe->Scored && BIRD_X > nearestPipe->X + PIPE_W)
            {
                nearestPipe->Scored = true;
                for (auto& otherBird : m_Birds)
                    if (otherBird.Alive) { otherBird.Fitness += 10.0; ++otherBird.PipesCleared; }
            }
            bird.Fitness += physDt;

            if (Collides(bird))
            {
                bird.Alive = false;
                --m_AliveCount;
            }
        }
    }

    m_SimTime += dt;
    if (m_AliveCount == 0) m_Done = true;
}

void FlappyBird::Render()
{
    SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);
    DrawSky();
    DrawGround();
    DrawPipes();
    DrawBirds();
}

double FlappyBird::BestLiveFitness() const
{
    double best = 0.0;
    for (const auto& bird : m_Birds)
        if (bird.Alive) best = std::max(best, bird.Fitness);
    return best;
}

void FlappyBird::Reset()
{
    m_Birds.clear();
    m_AliveCount = 0;
    m_Done = false;
    m_SimTime = 0.0;
    SpawnPipes();
}

bool FlappyBird::Collides(const Bird& bird) const
{
    if (bird.Y - BIRD_R <= 0.0f || bird.Y + BIRD_R >= m_GroundY) return true;
    for (const auto& pipe : m_Pipes)
    {
        const bool inHorizontalRange = BIRD_X + BIRD_R > pipe.X && BIRD_X - BIRD_R < pipe.X + PIPE_W;
        if (!inHorizontalRange) continue;
        if (bird.Y - BIRD_R < pipe.TopBot()) return true;
        if (bird.Y + BIRD_R > pipe.BottomTop()) return true;
    }
    return false;
}

void FlappyBird::SpawnPipes()
{
    m_Pipes.clear();
    std::uniform_real_distribution<float> yDistrib(270.0f, m_GroundY - 270.0f);
    for (uint32_t pipeIndex = 0; pipeIndex < N_PIPES; ++pipeIndex)
        m_Pipes.push_back({ (float)m_GameW + 150.0f + pipeIndex * PIPE_SPACING, yDistrib(m_Rng), false });
}

void FlappyBird::AdvancePipes(float dt)
{
    float rightmost = m_Pipes[0].X;
    for (const auto& pipe : m_Pipes) rightmost = std::max(rightmost, pipe.X);

    std::uniform_real_distribution<float> yDistrib(270.0f, m_GroundY - 270.0f);
    for (auto& pipe : m_Pipes)
    {
        pipe.X -= PIPE_SPEED * dt;
        if (pipe.X + PIPE_W < 0.0f)
        {
            pipe.X = rightmost + PIPE_SPACING;
            pipe.GapY = yDistrib(m_Rng);
            pipe.Scored = false;
            rightmost = pipe.X;
        }
    }
}

FlappyBird::Pipe* FlappyBird::NearestPipe()
{
    Pipe* nearestPipe = nullptr;
    for (auto& pipe : m_Pipes)
        if (pipe.X + PIPE_W > BIRD_X - BIRD_R)
            if (!nearestPipe || pipe.X < nearestPipe->X) nearestPipe = &pipe;
    return nearestPipe;
}

const FlappyBird::Pipe* FlappyBird::NearestPipeConst() const
{
    const Pipe* nearestPipe = nullptr;
    for (const auto& pipe : m_Pipes)
        if (pipe.X + PIPE_W > BIRD_X - BIRD_R)
            if (!nearestPipe || pipe.X < nearestPipe->X) nearestPipe = &pipe;
    return nearestPipe;
}

Drawer::Col FlappyBird::RandomColor()
{
    static const Drawer::Col palette[] = {
        {100, 220, 140, 210}, {100, 180, 255, 210}, {255, 180, 80, 210},
        {200, 100, 255, 210}, {255, 120, 140, 210}, {80, 220, 200, 210},
    };
    std::uniform_int_distribution<uint32_t> dist(0, 5);
    return palette[dist(m_Rng)];
}

void FlappyBird::DrawSky()
{
    for (uint32_t y = 0; y < m_WinH; ++y)
    {
        const float factor = (float)y / (float)m_WinH;
        SDL_SetRenderDrawColor(m_Renderer,
            static_cast<uint8_t>(18.0f + factor * 8.0f),
            static_cast<uint8_t>(22.0f + factor * 12.0f),
            static_cast<uint8_t>(45.0f + factor * 18.0f),
            255);
        SDL_RenderLine(m_Renderer, 0.0f, (float)y, (float)m_GameW, (float)y);
    }
}

void FlappyBird::DrawGround()
{
    Drawer::SetColor(m_Renderer, { 40, 160, 60, 255 });
    Drawer::FillRect(m_Renderer, 0.0f, m_GroundY, (float)m_GameW, (float)m_WinH - m_GroundY);
    Drawer::SetColor(m_Renderer, { 30, 120, 45, 255 });
    Drawer::FillRect(m_Renderer, 0.0f, m_GroundY, (float)m_GameW, 12.0f);
    Drawer::SetColor(m_Renderer, { 50, 180, 70, 60 });
    for (float groundX = 0.0f; groundX < (float)m_GameW; groundX += 45.0f)
        SDL_RenderLine(m_Renderer, groundX, m_GroundY + 12.0f, groundX + 22.0f, m_GroundY + 30.0f);
}

void FlappyBird::DrawPipes()
{
    for (const auto& pipe : m_Pipes)
    {
        Drawer::SetColor(m_Renderer, { 50, 180, 70, 255 });
        Drawer::FillRect(m_Renderer, pipe.X, 0.0f, PIPE_W, pipe.TopBot());
        Drawer::FillRect(m_Renderer, pipe.X, pipe.BottomTop(), PIPE_W, m_GroundY - pipe.BottomTop());
        Drawer::SetColor(m_Renderer, { 40, 150, 55, 255 });
        Drawer::FillRect(m_Renderer, pipe.X - 6.0f, pipe.TopBot() - 30.0f, PIPE_W + 12.0f, 30.0f);
        Drawer::FillRect(m_Renderer, pipe.X - 6.0f, pipe.BottomTop(), PIPE_W + 12.0f, 30.0f);
        Drawer::SetColor(m_Renderer, { 80, 220, 100, 80 });
        Drawer::FillRect(m_Renderer, pipe.X + 6.0f, 0.0f, 9.0f, pipe.TopBot());
        Drawer::FillRect(m_Renderer, pipe.X + 6.0f, pipe.BottomTop(), 9.0f, m_GroundY - pipe.BottomTop());
    }
}

void FlappyBird::DrawBirds()
{
    const Bird* bestBird = nullptr;
    for (const auto& bird : m_Birds)
        if (bird.Alive && (!bestBird || bird.Fitness > bestBird->Fitness)) bestBird = &bird;
    if (!bestBird)
        for (const auto& bird : m_Birds)
            if (!bestBird || bird.Fitness > bestBird->Fitness) bestBird = &bird;

    for (const auto& bird : m_Birds)
    {
        if (bird.Alive) continue;
        Drawer::SetColor(m_Renderer, {
            static_cast<uint8_t>(bird.Color.r / 3),
            static_cast<uint8_t>(bird.Color.g / 3),
            static_cast<uint8_t>(bird.Color.b / 3),
            40
            });
        Drawer::DrawCircle(m_Renderer, BIRD_X, bird.Y, BIRD_R * 0.55f);
    }

    for (const auto& bird : m_Birds)
    {
        if (!bird.Alive) continue;
        const bool isBest = (&bird == bestBird);

        if (isBest)
        {
            Drawer::SetColor(m_Renderer, { 255, 240, 60, 35 });
            Drawer::DrawCircle(m_Renderer, BIRD_X, bird.Y, BIRD_R + 10.0f);
        }

        Drawer::SetColor(m_Renderer, isBest ? Drawer::Col{ 255, 230, 60, 235 } : bird.Color);
        Drawer::DrawCircle(m_Renderer, BIRD_X, bird.Y, BIRD_R);

        Drawer::SetColor(m_Renderer, { 10, 10, 20, 255 });
        Drawer::DrawCircle(m_Renderer, BIRD_X + 7.0f, bird.Y - 6.0f, 4.0f);
        Drawer::SetColor(m_Renderer, { 255, 255, 255, 255 });
        Drawer::DrawCircle(m_Renderer, BIRD_X + 8.0f, bird.Y - 7.0f, 2.0f);
    }
}