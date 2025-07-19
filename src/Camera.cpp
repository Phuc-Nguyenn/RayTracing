#include "Camera.h"
#include "Scene.h"
#include "Renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>

#define PI_ON_180 0.01745329251994329576923690768489

Camera::Camera(Scene& scene) : 
    position(DEFAULT_POSITION), 
    facing(DEFAULT_FACING), 
    fov(degreesToRadians(DEFAULT_FOV_DEGREES)),
    viewportDistance(VIEWPORT_DISTANCE),
    observer([this](const KeyEvent& event) { this->OnEvent(event); }),
    scene(scene)
{
    viewportWidth = 2.0 * viewportDistance * tan(this->fov / 2.0);
};

Vector3f Camera::GetPosition() const{
    return position;
}

Vector3f Camera::GetFacing() const{
    return facing;
}

float Camera::GetFov() const{
    return fov;
}

float Camera::GetViewportWidth() const{
    return viewportWidth;
}

float Camera::GetViewportDistance() const{
    return viewportDistance;
}

void Camera::SetPosition(Vector3f newPosition) {
    position = newPosition;
}

void Camera::SetFacing(Vector3f newFacing) {
    facing = newFacing.Normalize();
}

void Camera::SetFov(float newFov) {
    fov = newFov;
}

void Camera::SetViewportWidth(float newViewportWidth) {
    viewportWidth = newViewportWidth;
}

void Camera::SetViewportDistance(float newViewportDistance) {
    viewportDistance = newViewportDistance;
}
//helpers
double Camera::degreesToRadians(double degrees) {
    return degrees * PI_ON_180;
}

Vector3f Camera::RotateAroundAxis(const Vector3f& v, const Vector3f& k, float a) {
    float cosA = cos(a);
    float sinA = sin(a);
    Vector3f result = v * cosA + k.Cross(v) * sinA + k * k.Dot(v) * (1 - cosA);
    return result;
}

void Camera::SetFOV(float degrees) {
    this->fov = degreesToRadians(degrees);
    viewportWidth = 2.0 * viewportDistance * tan(this->fov / 2.0);
};

void Camera::Tick(float cameraSpeed, float rotationSpeed) {
    Vector3f cameraInitialPosition = position;
    Vector3f cameraInitialFacing = facing;
    float cameraInitialFOV = fov;
    
    Vector3f right = facing.Cross({0.0f, 0.0f, 1.0f}).Normalize();  // Right vector
    Vector3f up = right.Cross(facing).Normalize();
    
    if (keys[GLFW_KEY_LEFT_SHIFT]) {
        cameraSpeed *= 2;
        rotationSpeed *= 2;
    } else if (keys[GLFW_KEY_RIGHT_SHIFT]) {
        cameraSpeed /= 2;
        rotationSpeed /= 2;
    }

    if(keys[GLFW_KEY_W])
        position = position + facing * cameraSpeed;
    if(keys[GLFW_KEY_S])
        position = position - facing * cameraSpeed; 
    if(keys[GLFW_KEY_A]) 
        position = position - right * cameraSpeed; 
    if(keys[GLFW_KEY_D]) 
        position = position + right * cameraSpeed; 
    if(keys[GLFW_KEY_SPACE]) 
        position = position + Vector3f(0,0,1) * (cameraSpeed/2); 
    if(keys[GLFW_KEY_LEFT_CONTROL])
        position = position - Vector3f(0,0,1) * (cameraSpeed/2); 
        
    // rotations
    if(keys[GLFW_KEY_UP]) 
        facing = RotateAroundAxis(facing, right, rotationSpeed); 
    if(keys[GLFW_KEY_DOWN]) 
        facing = RotateAroundAxis(facing, right, -rotationSpeed);
    if(keys[GLFW_KEY_LEFT]) 
        facing = RotateAroundAxis(facing, up, rotationSpeed); 
    if(keys[GLFW_KEY_RIGHT]) 
        facing = RotateAroundAxis(facing, up, -rotationSpeed); 
    // zoom
    if(keys[GLFW_KEY_C])
        SetFOV(ZOOM_FOV_DEGREES);
    else
        SetFOV(DEFAULT_FOV_DEGREES);

    bool cameraMoved = !(cameraInitialFacing == facing) || !(cameraInitialPosition == position) || !(cameraInitialFOV == fov);
    if(cameraMoved) {
        UploadInfo();
        scene.ResetFrameIndex();
    }
}

void Camera::UploadInfo() {
    unsigned int programId = scene.GetShaderProgramIds()[TRACER_ID];
    glUniform3f(glGetUniformLocation(programId, "u_Camera.position"), position.x, position.y, position.z);
    glUniform3f(glGetUniformLocation(programId, "u_Camera.facing"), facing.x, facing.y, facing.z);
    glUniform1f(glGetUniformLocation(programId, "u_Camera.fov"), fov);
    glUniform1f(glGetUniformLocation(programId, "u_Camera.viewportWidth"), viewportWidth);
}

void Camera::OnEvent(const KeyEvent& keyEvent) {
    if(keyEvent.action == GLFW_PRESS) {
        keys[keyEvent.key] = true;
    } else if(keyEvent.action == GLFW_RELEASE) {
        keys[keyEvent.key] = false;
    }
}

Scene& Camera::GetScene() {
    return scene;
};