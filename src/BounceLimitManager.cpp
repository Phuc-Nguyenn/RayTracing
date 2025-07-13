#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "BounceLimitManager.h"
#include "Scene.h"
#include "Renderer.h"

BounceLimitManager::BounceLimitManager(Scene& scene, unsigned int& shaderProgramId) :
    scene(scene),
    shaderProgramId(shaderProgramId), 
    observer([this](const KeyEvent& event) { this->OnEvent(event); })
{}

void BounceLimitManager::OnEvent(const KeyEvent& event) {
    if(event.action != GLFW_PRESS) {
        return;
    }
    switch(event.key) {
    case GLFW_KEY_GRAVE_ACCENT:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 0); // view boxes
        scene.ResetFrameIndex();
        break;
    case GLFW_KEY_1:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 1); // view normals
        scene.ResetFrameIndex();
        return;
    case GLFW_KEY_2:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 2);
        scene.ResetFrameIndex();
        return;
    case GLFW_KEY_3:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 4);
        scene.ResetFrameIndex();
        return;
    case GLFW_KEY_4:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 8);
        scene.ResetFrameIndex();
        return;
    case GLFW_KEY_5:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 16);
        scene.ResetFrameIndex();
        return;
    case GLFW_KEY_6:
        glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 32);
        scene.ResetFrameIndex();
        return;
    }
};


