#pragma once

#include <chrono>

namespace Utils
{

unsigned int TimeSinceMs(const std::chrono::steady_clock::time_point& pastTime) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - pastTime).count();
}

} // utils