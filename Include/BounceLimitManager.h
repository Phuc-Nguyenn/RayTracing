#pragma once

#include "KeyEventObserver.h"

class Scene; // forward declaration

class BounceLimitManager {
private:
    KeyEventObserver observer;
    unsigned int& shaderProgramId;
    Scene& scene;

    void OnEvent(const KeyEvent& event);

public:
    BounceLimitManager(Scene& scene, unsigned int& shaderProgramId);
};