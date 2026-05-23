#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "ERP.h"

namespace Drawer
{
	static constexpr float SIDEBAR_WIDTH = 380.0f;

	static constexpr uint32_t DEFAULT_WINDOW_WIDTH = 1920;
	static constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 1080;

	inline TTF_Font* g_FontSmall = nullptr;
	inline TTF_Font* g_FontMedium = nullptr;

	struct Col
	{
		uint8_t r, g, b, a;
	};

	Col LerpCol(Col from, Col to, float factor);

	void DrawTextSlow(SDL_Renderer* renderer, std::string_view text, float x, float y, Col color, TTF_Font* font, bool center = false);
	void SetColor(SDL_Renderer* renderer, Col color);
	void FillRect(SDL_Renderer* renderer, float x, float y, float w, float h);
	void DrawRect(SDL_Renderer* renderer, float x, float y, float w, float h);
	void DrawCircle(SDL_Renderer* renderer, float cx, float cy, float rad);
};