#pragma once

#include <execution>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <string>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <limits>
#include <utility>
#include <bitset>

#define NOMINMAX

static constexpr double SIM_DT = 1.0 / 60.0; // Seconds
static constexpr uint32_t NEURON_SUBSTEPS = 8;
static constexpr uint32_t SERIAL_SUBSTEPS = 32;

static constexpr double g_PI = 3.1415926535897932384626433832;

static uint32_t HashSeed(uint32_t generation, uint32_t slot)
{
    uint32_t h = generation * 2654435761u + slot * 40503u + 0x9E3779B9u;
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
    return h;
}

static void Assert(bool condition)
{
    if (!condition)
    {
        __debugbreak();
        exit(1);
    }
}