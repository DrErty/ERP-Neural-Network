#pragma once

#include "ERP.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Evolution.h"
#include "Drawer.h"

void DrawSidebar(SDL_Renderer* renderer, uint32_t generation, const std::vector<Individual>& individuals, Game& game, const std::vector<double>& history, double sigma, double frameTime);