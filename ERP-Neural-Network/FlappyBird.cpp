#include "FlappyBird.h"

#include "ERP.h"

FlappyBird::FlappyBird(SDL_Renderer* renderer, std::mt19937& rng, uint32_t gameHeight, uint32_t gameWidth)
    : m_Renderer(renderer)
    , m_Rng(rng)
    , m_GameHeight(gameHeight)
    , m_GameWidth(gameWidth)
    , m_GroundY(static_cast<float>(gameHeight) - 90.0f)
{
    SpawnPipes();
}

uint32_t FlappyBird::AddPlayer(bool display)
{
    const uint32_t playerIndex = (uint32_t)m_Birds.size();
    Bird bird;
    bird.Y = (float)m_GameHeight * 0.45f;
    m_Birds.push_back(std::move(bird));
    ++m_AliveCount;
    return playerIndex;
}

void FlappyBird::Action(uint32_t playerIndex, uint32_t outputIndex)
{
    Assert(playerIndex < m_Birds.size());
    Assert(outputIndex < OUTPUT_NEURONS);

    if (outputIndex == 0)
    {
        if (m_Birds[playerIndex].Alive)
            m_Birds[playerIndex].Vy = FLAP_VEL;
    }
}

float FlappyBird::GetInput(uint32_t playerIndex, uint32_t inputIndex) const
{
    Assert(playerIndex < m_Birds.size());

    if (inputIndex == 0)
    {
        const Pipe* const nearestPipe = NearestPipeConst();
        if (!nearestPipe) return 0.0f;
        return nearestPipe->GapY - m_Birds[playerIndex].Y;
    }
    if (inputIndex == 1)
    {
        return m_Birds[playerIndex].Vy;
    }
    if (inputIndex == 2)
    {
        return -m_Birds[playerIndex].Vy;
    }
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

            if (nearestPipe)
            {
                float extraScore = std::clamp(1.0f - (std::abs(nearestPipe->GapY - bird.Y) / PIPE_GAP), 0.0f, 1.0f);
                bird.Fitness += extraScore * physDt * 100.0;
            }

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

void FlappyBird::Reset()
{
    m_Birds.clear();
    m_AliveCount = 0;
    m_Done = false;
    m_SimTime = 0.0;
    SpawnPipes();
}

void FlappyBird::KillPlayer(uint32_t playerIndex)
{
    if (m_Birds[playerIndex].Alive)
    {
        m_Birds[playerIndex].Alive = false;
        --m_AliveCount;
    }
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
        m_Pipes.push_back({ (float)m_GameWidth + 150.0f + pipeIndex * PIPE_SPACING, yDistrib(m_Rng), false });
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

void FlappyBird::DrawSky()
{
    for (uint32_t y = 0; y < m_GameHeight; ++y)
    {
        const float factor = (float)y / (float)m_GameHeight;
        SDL_SetRenderDrawColor(m_Renderer,
            static_cast<uint8_t>(18.0f + factor * 8.0f),
            static_cast<uint8_t>(22.0f + factor * 12.0f),
            static_cast<uint8_t>(45.0f + factor * 18.0f),
            255);
        SDL_RenderLine(m_Renderer, 0.0f, (float)y, (float)m_GameWidth, (float)y);
    }
}

void FlappyBird::DrawGround()
{
    Drawer::SetColor(m_Renderer, { 40, 160, 60, 255 });
    Drawer::FillRect(m_Renderer, 0.0f, m_GroundY, (float)m_GameWidth, (float)m_GameHeight - m_GroundY);
    Drawer::SetColor(m_Renderer, { 30, 120, 45, 255 });
    Drawer::FillRect(m_Renderer, 0.0f, m_GroundY, (float)m_GameWidth, 12.0f);
    Drawer::SetColor(m_Renderer, { 50, 180, 70, 60 });
    for (float groundX = 0.0f; groundX < (float)m_GameWidth; groundX += 45.0f)
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

#if 0
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
#endif

    m_SortedBirds.clear();
    for (const auto& bird : m_Birds)
    {
        if (!bird.Alive) continue;
        m_SortedBirds.emplace_back(&bird);
    }

    std::sort(m_SortedBirds.begin(), m_SortedBirds.end(), [&](const Bird* a, const Bird* b)
        {
            return a->Fitness > b->Fitness;
        });

    uint32_t drawCount = 0;
    for (const Bird* bird : m_SortedBirds)
    {
        if (drawCount > MAX_BIRD_DRAW_COUNT)
            break;

        Drawer::SetColor(m_Renderer, Drawer::Col{ 255, 230, 60, 235 });
        Drawer::DrawCircle(m_Renderer, BIRD_X, bird->Y, BIRD_R);

        Drawer::SetColor(m_Renderer, { 10, 10, 20, 255 });
        Drawer::DrawCircle(m_Renderer, BIRD_X + 7.0f, bird->Y - 6.0f, 4.0f);
        Drawer::SetColor(m_Renderer, { 255, 255, 255, 255 });
        Drawer::DrawCircle(m_Renderer, BIRD_X + 8.0f, bird->Y - 7.0f, 2.0f);

        drawCount++;
    }
}