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

#define NOMINMAX

static constexpr double g_PI = 3.1415926535897932384626433832;

static void Assert(bool condition)
{
    if (!condition)
    {
        __debugbreak();
        exit(1);
    }
}