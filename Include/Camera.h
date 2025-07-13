#pragma once

#include "Math3D.h"
#include "KeyEventObserver.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>

// camera setting macros
#define VIEWPORT_DISTANCE 1.0
#define DEFAULT_POSITION {0,0,0}
#define DEFAULT_FACING {1,0,0}
#define DEFAULT_FOV_DEGREES 150
#define ZOOM_FOV_DEGREES 90
#define GLFW_KEY_COUNT 350

// forward declare scene class
class Scene;

class Camera {
private:
    Vector3f position;
    Vector3f facing;
    float fov;
    float viewportWidth;
    float viewportDistance;
    KeyEventObserver observer;
    Scene& scene;
    bool keys[GLFW_KEY_COUNT] = {false};
    
    void OnEvent(const KeyEvent& keyEvent);
    
    
    
    //helpers
    double degreesToRadians(double degrees);

    Vector3f RotateAroundAxis(const Vector3f& v, const Vector3f& k, float a);

    void SetFOV(float fov);

public:
    Camera(Scene& scene);

    Vector3f GetPosition() const;

    Vector3f GetFacing() const;

    float GetFov() const;

    float GetViewportWidth() const;

    float GetViewportDistance() const;

    Scene& GetScene();

    void SetPosition(Vector3f newPosition);

    void SetFacing(Vector3f newFacing);

    void SetFov(float newFov);

    void SetViewportWidth(float newViewportWidth);

    void SetViewportDistance(float newViewportDistance);
    
    void Tick(float cameraSpeed = 0.2, float rotationSpeed = 0.02f);
    
    void UploadInfo();
};
