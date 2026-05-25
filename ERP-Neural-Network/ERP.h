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

static void Assert(bool condition)
{
    if (!condition)
    {
        __debugbreak();
        exit(1);
    }
}