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
#include <span>
#include <functional>
#include <charconv>
#include <format>
#include <fstream>
#include <locale>

#define NOMINMAX

#include "Logger.h"

using Scalar = double;

static constexpr Scalar g_PI = Scalar(3.1415926535897932384626433832);

static constexpr Scalar SIM_DT = Scalar(1.0 / 60.0); // Seconds

static constexpr uint32_t NEURON_SUBSTEPS = 16;
static constexpr uint32_t SERIAL_SUBSTEPS = 32;

static constexpr uint32_t MAX_INDIVIDUALS = 512;
static constexpr uint32_t EVOLUTION_MU = 32;
static constexpr uint32_t MAX_EVALUTIONS_PER_GENOME = 8;

static constexpr Scalar INITIAL_SIGMA = Scalar(10.0);
//static constexpr Scalar INITIAL_NEW_WEIGHT_SIGMA = Scalar(1.0);

static constexpr float CROSSOVER_CHANCE = 0.25f;
static constexpr float NEW_CONNECTION_CHANCE = 0.8f;
static constexpr float DELETE_CONNECTION_CHANCE = 0.5f;

static constexpr Scalar MAX_GAME_TIME = Scalar(60.0);

static constexpr bool MUTABLE_TOPOLOGY = false;

static constexpr Scalar MAX_OUTPUT_RATE_OFFSET = Scalar(20.0);
static constexpr Scalar MIN_INPUT_RATE = Scalar(0.0);
static constexpr Scalar MAX_INPUT_RATE = Scalar(60.0);

static constexpr uint32_t MAX_MEASUREMENTS = 10;

template<typename... Args>
static void ExitFatal(const char* file, uint32_t line, Args&&... args)
{
	const char* const lastSeperator = strrchr(file, '\\');
	Logger::PrintLine("[ERROR]: ", lastSeperator ? lastSeperator + 1 : file, "(", line, "): ", std::forward<Args>(args)...);
	__debugbreak();
	exit(EXIT_FAILURE);
}

#define ERP_VERBOSE

#define ERP_VERIFY(check, ...) if (!(check)) ExitFatal(__FILE__, __LINE__, __VA_ARGS__)

#ifdef _DEBUG
#define ERP_ASSERT(check, ...) if (!(check)) ExitFatal(__FILE__, __LINE__, __VA_ARGS__)
#else
#define ERP_ASSERT(check, ...)
#endif

#ifdef ERP_VERBOSE
#define ERP_LOG(...) Logger::PrintLine("[VERBOSE]: ", __VA_ARGS__)
#define ERP_LOG_ERROR(...) Logger::PrintLine("[ERROR]: ", __VA_ARGS__)
#else
#define ERP_LOG(...)
#define ERP_LOG_ERROR(...)
#endif

#define ERP_EXIT_FATAL(...) ExitFatal(__FILE__, __LINE__, __VA_ARGS__)

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