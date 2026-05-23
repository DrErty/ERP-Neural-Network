#include "Drawer.h"

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
}