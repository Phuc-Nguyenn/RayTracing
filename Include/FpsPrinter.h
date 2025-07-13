#pragma once

#include <chrono>
#include <iostream>

class FpsPrinter {
public:
    FpsPrinter() : 
        lastFpsCheck(std::chrono::steady_clock::now()),
        secFrameCount(0)
    {

    };

    void Tick() {
        auto now = std::chrono::steady_clock::now();
        secFrameCount++;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsCheck).count() > 1000) {
            lastFpsCheck = now;
            unsigned int fps = secFrameCount;
            std::cout << "fps: " << fps << std::endl;
            secFrameCount = 0;
        }
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> lastFpsCheck;
    unsigned int secFrameCount;
};