#include "Drawer.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <array>

namespace Drawer
{
    Col LerpCol(Col from, Col to, float factor)
    {
        factor = std::clamp(factor, 0.0f, 1.0f);
        return {
            static_cast<uint8_t>(from.r + factor * (to.r - from.r)),
            static_cast<uint8_t>(from.g + factor * (to.g - from.g)),
            static_cast<uint8_t>(from.b + factor * (to.b - from.b)),
            static_cast<uint8_t>(from.a + factor * (to.a - from.a))
        };
    }

    void DrawTextSlow(SDL_Renderer* renderer, std::string_view text, float x, float y, Col color, TTF_Font* font, bool center)
    {
        const SDL_Color sdlColor = { color.r, color.g, color.b, color.a };
        SDL_Surface* const surface = TTF_RenderText_Blended(font, text.data(), text.length(), sdlColor);
        if (!surface) return;
        SDL_Texture* const texture = SDL_CreateTextureFromSurface(renderer, surface);
        const float textWidth = (float)surface->w;
        const float textHeight = (float)surface->h;
        SDL_DestroySurface(surface);
        if (!texture) return;
        const SDL_FRect dst = { center ? x - textWidth * 0.5f : x, y, textWidth, textHeight };
        SDL_RenderTexture(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    void SetColor(SDL_Renderer* renderer, Col color)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    void FillRect(SDL_Renderer* renderer, float x, float y, float w, float h)
    {
        const SDL_FRect rect = { x, y, w, h };
        SDL_RenderFillRect(renderer, &rect);
    }

    void DrawRect(SDL_Renderer* renderer, float x, float y, float w, float h)
    {
        const SDL_FRect rect = { x, y, w, h };
        SDL_RenderRect(renderer, &rect);
    }

    void DrawCircle(SDL_Renderer* renderer, float cx, float cy, float rad)
    {
        const int32_t radius = (int32_t)rad;
        for (int32_t dy = -radius; dy <= radius; ++dy)
        {
            const float dx = std::sqrt((float)(radius * radius - dy * dy));
            SDL_RenderLine(renderer, cx - dx, cy + (float)dy, cx + dx, cy + (float)dy);
        }
    }

    void DrawSidebar(SDL_Renderer* renderer)
    {
        uint32_t m_AliveCount = 1;
        uint32_t BirdCount = 1;
        uint32_t m_Generation = 0;
        double m_BestEver = 0.0;
        double m_Sigma = 0.0;
        std::vector<double> m_History;

        double BestLiveFitness = 0.0;

        uint32_t m_GameW = DEFAULT_WINDOW_WIDTH - SIDEBAR_WIDTH;
        uint32_t m_GameH = DEFAULT_WINDOW_HEIGHT;

        uint32_t m_WinW = DEFAULT_WINDOW_WIDTH;
        uint32_t m_WinH = DEFAULT_WINDOW_HEIGHT;

        const float sidebarX = (float)m_GameW;
        const float posX = sidebarX + 18.0f;
        float posY = 14.0f;
        const float barWidth = SIDEBAR_WIDTH - 36.0f;

        Drawer::SetColor(renderer, { 8, 10, 18, 255 });
        Drawer::FillRect(renderer, sidebarX, 0.0f, SIDEBAR_WIDTH, (float)m_WinH);
        Drawer::SetColor(renderer, { 40, 50, 80, 255 });
        SDL_RenderLine(renderer, sidebarX, 0.0f, sidebarX, (float)m_WinH);

        Drawer::DrawTextSlow(renderer, "NEURO FLAPPY", posX, posY, { 180, 200, 255, 220 }, Drawer::g_FontMedium);
        posY += Drawer::g_FontMedium ? 42.0f : 28.0f;

        Drawer::DrawTextSlow(renderer, "Generation", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;

        Drawer::DrawTextSlow(renderer, std::to_string(m_Generation), posX, posY, { 255, 220, 60, 255 }, Drawer::g_FontMedium);
        posY += Drawer::g_FontMedium ? 44.0f : 30.0f;

        Drawer::DrawTextSlow(renderer, "Alive", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;
        const float aliveFraction = BirdCount > 0 ? (float)m_AliveCount / (float)BirdCount : 0.0f;
        Drawer::SetColor(renderer, { 25, 30, 55, 255 });
        Drawer::FillRect(renderer, posX, posY, barWidth, 20.0f);
        Drawer::SetColor(renderer, Drawer::LerpCol({ 220, 60, 60, 255 }, { 60, 220, 120, 255 }, aliveFraction));
        Drawer::FillRect(renderer, posX, posY, barWidth * aliveFraction, 20.0f);
        Drawer::SetColor(renderer, { 15, 18, 35, 255 });
        for (uint32_t tickIndex = 1; tickIndex < 10; ++tickIndex)
            Drawer::FillRect(renderer, posX + barWidth * (float)tickIndex * 0.1f - 0.5f, posY, 1.0f, 20.0f);
        posY += 28.0f;
        Drawer::DrawTextSlow(renderer, std::to_string(m_AliveCount) + " / " + std::to_string(BirdCount),
            posX, posY, { 160, 200, 255, 220 }, Drawer::g_FontSmall);
        posY += 28.0f;

        Drawer::DrawTextSlow(renderer, "Best fitness", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;
        std::array<char, 32> textBuffer;
        std::snprintf(textBuffer.data(), textBuffer.size(), "%.1f", BestLiveFitness);
        Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 120, 255, 160, 255 }, Drawer::g_FontMedium);
        posY += Drawer::g_FontMedium ? 44.0f : 30.0f;

        Drawer::DrawTextSlow(renderer, "All-time best", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;
        std::snprintf(textBuffer.data(), textBuffer.size(), "%.1f", m_BestEver);
        Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 255, 220, 60, 200 }, Drawer::g_FontSmall);
        posY += 30.0f;

        Drawer::DrawTextSlow(renderer, "Sigma", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
        posY += 20.0f;
        const float sigmaFraction = std::clamp((float)m_Sigma / 3.0f, 0.0f, 1.0f);
        Drawer::SetColor(renderer, { 25, 30, 55, 255 });
        Drawer::FillRect(renderer, posX, posY, barWidth, 12.0f);
        Drawer::SetColor(renderer, Drawer::LerpCol({ 60, 120, 255, 180 }, { 255, 120, 60, 180 }, sigmaFraction));
        Drawer::FillRect(renderer, posX, posY, barWidth * sigmaFraction, 12.0f);
        std::snprintf(textBuffer.data(), textBuffer.size(), "%.3f", m_Sigma);
        posY += 16.0f;
        Drawer::DrawTextSlow(renderer, textBuffer.data(), posX, posY, { 160, 160, 180, 180 }, Drawer::g_FontSmall);
        posY += 28.0f;

        if (m_History.size() > 1)
        {
            Drawer::DrawTextSlow(renderer, "Fitness history", posX, posY, { 120, 140, 180, 200 }, Drawer::g_FontSmall);
            posY += 20.0f;
            const float sparkHeight = 90.0f;
            Drawer::SetColor(renderer, { 20, 25, 48, 255 });
            Drawer::FillRect(renderer, posX, posY, barWidth, sparkHeight);
            Drawer::DrawRect(renderer, posX, posY, barWidth, sparkHeight);

            const double maxFitness = std::max(
                *std::max_element(m_History.begin(), m_History.end()), 1.0);
            const uint32_t sampleCount = (uint32_t)std::min((size_t)barWidth, m_History.size());

            for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
            {
                const uint32_t historyIndex = (uint32_t)m_History.size() - sampleCount + sampleIndex;
                const float sparkX = posX + (float)sampleIndex * barWidth / (float)sampleCount;
                const float sparkY = posY + sparkHeight - (float)(m_History[historyIndex] / maxFitness * sparkHeight);
                const float alpha = 0.3f + 0.7f * (float)sampleIndex / (float)sampleCount;
                Drawer::SetColor(renderer, { 60, 160, 100, static_cast<uint8_t>(50.0f * alpha) });
                SDL_RenderLine(renderer, sparkX, sparkY, sparkX, posY + sparkHeight);
            }
            for (uint32_t sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex)
            {
                const uint32_t historyIndex = (uint32_t)m_History.size() - sampleCount + sampleIndex;
                const float startX = posX + (float)(sampleIndex - 1) * barWidth / (float)sampleCount;
                const float endX = posX + (float)sampleIndex * barWidth / (float)sampleCount;
                const float startY = posY + sparkHeight - (float)(m_History[historyIndex - 1] / maxFitness * sparkHeight);
                const float endY = posY + sparkHeight - (float)(m_History[historyIndex] / maxFitness * sparkHeight);
                Drawer::SetColor(renderer, { 100, 220, 160, 220 });
                SDL_RenderLine(renderer, startX, startY, endX, endY);
            }
            const float bestLine = posY + sparkHeight - (float)(m_BestEver / maxFitness * sparkHeight);
            Drawer::SetColor(renderer, { 255, 220, 60, 100 });
            SDL_RenderLine(renderer, posX, bestLine, posX + barWidth, bestLine);
            posY += sparkHeight + 12.0f;
        }

        Drawer::SetColor(renderer, { 60, 70, 100, 200 });
        Drawer::FillRect(renderer, sidebarX, (float)m_WinH - 36.0f, SIDEBAR_WIDTH, 36.0f);
        Drawer::DrawTextSlow(renderer, "[ESC] quit   [R] pause", posX, (float)m_WinH - 26.0f,
            { 120, 130, 160, 200 }, Drawer::g_FontSmall);
    }
}