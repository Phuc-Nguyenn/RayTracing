#pragma once

#include <chrono>
#include <iostream>
#include "Camera.h"

class InfoPrinter {
public:
    InfoPrinter(const Camera& camera) : 
        lastFpsCheck(std::chrono::steady_clock::now()),
        secFrameCount(0),
        camera(camera)
    {
    };

    void Tick() {
        auto now = std::chrono::steady_clock::now();
        secFrameCount++;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsCheck).count() > 1000) {
            lastFpsCheck = now;
            unsigned int fps = secFrameCount;
            std::cout << "fps: " << fps << ", ";
            secFrameCount = 0;
            
            // print camera position
            Vector3f position = camera.GetPosition();
            std::cout << "position: " << position.x << " " << position.y << " " << position.z;
            std::cout << std::endl;
        }
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> lastFpsCheck;
    unsigned int secFrameCount;
    const Camera& camera;
};