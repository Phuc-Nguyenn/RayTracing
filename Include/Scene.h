#pragma once

#include "Math3D.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <cmath>
#include "Materials.h"

#define SPHERE 0
#define PLANE 1

#define VIEWPORT_DISTANCE 1.0

using namespace Material;

double degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

struct Camera {
    Vector3f position;
    Vector3f facing;
    float fov;
    float viewportWidth;
    float viewportDistance;


    Camera(Vector3f pos, Vector3f facing, float fov) : position(pos), facing(facing), fov(degreesToRadians(fov)), viewportDistance(VIEWPORT_DISTANCE){
        viewportWidth = 2.0 * viewportDistance * tan(this->fov / 2.0);
    };

    void SetFOVTo(float fov) {
        this->fov = degreesToRadians(fov);
        viewportWidth = 2.0 * viewportDistance * tan(this->fov / 2.0);
    };
};

Vector3f RotateAroundAxis(const Vector3f& v, const Vector3f& k, float a) {
    float cosA = cos(a);
    float sinA = sin(a);
    Vector3f result = v * cosA + k.Cross(v) * sinA + k * k.Dot(v) * (1 - cosA);
    return result;
}

#include <iostream>

class Scene {
    private:
        
        unsigned int objectsIndex;
        unsigned int shaderProgramId;
        unsigned int lightsIndex;
        unsigned int frameIndex;
        Camera camera;
        
    public:
        
        Scene(unsigned int shaderProgramId) : shaderProgramId(shaderProgramId), objectsIndex(0), lightsIndex(0), camera{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 90}, frameIndex(0) {
        }

        void AddSphere(Vector3f position, float radius, const Material::Material& material) {
            SetNextObject(SPHERE, position, {0.0, 0.0, 0.0}, radius, std::move(material));
        }

        void AddPlane(Vector3f position, Vector3f normal, const Material::Material& material) {
            SetNextObject(PLANE, position, normal, 0.0, std::move(material));
        }

        void Finalise() {
            glUniform3f(GetUniformLocation("u_Camera.position"), camera.position.x, camera.position.y, camera.position.z);
            glUniform3f(GetUniformLocation("u_Camera.facing"), camera.facing.x, camera.facing.y, camera.facing.z);
            glUniform1f(GetUniformLocation("u_Camera.fov"), camera.fov);
            glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.viewportWidth);
            glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.viewportDistance);

            auto objectCountLoc = GetUniformLocation("u_HittablesCount");
            glUniform1ui(objectCountLoc, objectsIndex);

            auto lightCountLoc = GetUniformLocation("u_LightsCount");
            glUniform1ui(lightCountLoc, lightsIndex);

            glUniform1ui(glGetUniformLocation(shaderProgramId, "u_FrameIndex"), frameIndex++);
            glUniform3f(glGetUniformLocation(shaderProgramId, "u_RandSeed"), std::rand(), std::rand(), std::rand());
        }

        void ResetFrameIndex() {
            frameIndex = 0;
        }

        void SetNextObject(unsigned int type, const Vector3f& position, const Vector3f& dir, float scale, const Material::Material& material) {
            auto typeLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"type"}});
            auto positionLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"position"}});
            auto directionLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"direction"}});
            auto scaleLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"scale"}});
            auto colourLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"colour"}});
            auto reflectivityLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"reflectivity"}});
            auto roughnessLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"roughness"}});
            auto transparentLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"transparent"}});
            auto refractionIdxLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"refractionIndex"}});
            auto isLightSourceLoc = GetUniformLocationIdx("u_Hittable", objectsIndex, {{"material"}, {"isLightSource"}});
           
            glUniform1ui(typeLoc, type);
            glUniform3f(positionLoc, position.x, position.y, position.z);
            glUniform3f(directionLoc, dir.x, dir.y, dir.z);
            glUniform1f(scaleLoc, scale);
            
            glUniform3f(colourLoc, material.colour.x, material.colour.y, material.colour.z);
            glUniform1f(reflectivityLoc, sqrt(material.reflectivity));
            glUniform1f(roughnessLoc, material.roughness);
            glUniform1i(transparentLoc, material.transparent);
            glUniform1f(refractionIdxLoc, material.refractionIndex);
            glUniform1i(isLightSourceLoc, material.isLightSource);
            objectsIndex++;
        }

        void SetCamera(Camera camera) {
            this->camera = std::move(camera);
            glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.viewportWidth);
            glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.viewportDistance);
            glUniform1f(GetUniformLocation("u_Camera.fov"), camera.fov);
        }

        unsigned int GetUniformLocation(std::string uname) {
            std::string query = uname;
            return glGetUniformLocation(shaderProgramId, query.c_str());
        }

        unsigned int GetUniformLocationIdx(std::string uname, unsigned int index, std::vector<std::string> attributes) {
            std::string indexStr = std::to_string(index);
            std::string query = uname + "[" + indexStr +"]";
            for(auto attribute : attributes) {
                query += "." + attribute;
            }
            return glGetUniformLocation(shaderProgramId, query.c_str());
        };

        void TranslateCamera(unsigned int keyPressed, float cameraSpeed = 0.25) {
            Vector3f right = camera.facing.Cross({0.0f, 0.0f, 1.0f}).Normalize();  // Right vector

            switch(keyPressed)
            {
                case GLFW_KEY_W:
                    camera.position = camera.position + camera.facing * cameraSpeed; break;
                case GLFW_KEY_S:
                    camera.position = camera.position - camera.facing * cameraSpeed; break;
                case GLFW_KEY_A:
                    camera.position = camera.position - right * cameraSpeed; break;
                case GLFW_KEY_D:
                    camera.position = camera.position + right * cameraSpeed; break;
                case GLFW_KEY_SPACE:
                    camera.position = camera.position + Vector3f(0,0,1) * (cameraSpeed/2); break;
                case GLFW_KEY_LEFT_CONTROL:
                    camera.position = camera.position - Vector3f(0,0,1) * (cameraSpeed/2); break;
            }
        }

        void RotateCameraFacing(unsigned int direction, float rotationSpeed = 0.02f) {  
            Vector3f right = camera.facing.Cross({0.0f, 0.0f, 1.0f}).Normalize();  // Right axis
                                                  // Global up axis
            Vector3f up = right.Cross(camera.facing).Normalize();

            switch (direction) {
                case GLFW_KEY_UP:
                    camera.facing = RotateAroundAxis(camera.facing, right, rotationSpeed); break;
                case GLFW_KEY_DOWN:
                    camera.facing = RotateAroundAxis(camera.facing, right, -rotationSpeed);break;
                case GLFW_KEY_LEFT:
                    camera.facing = RotateAroundAxis(camera.facing, up, rotationSpeed);break;
                case GLFW_KEY_RIGHT:
                    camera.facing = RotateAroundAxis(camera.facing, up, -rotationSpeed);break;
            }
        
            camera.facing = camera.facing.Normalize(); // Always re-normalize
        }

        bool HandleCameraMovement(bool keys[]) {

            float cameraSpeed = 0.2;
            float rotationSpeed = 0.02;
            if (keys[GLFW_KEY_LEFT_SHIFT]) {
                cameraSpeed = 0.5;
                rotationSpeed = 0.04;
            }
            
            Vector3f cameraInitialPosition = camera.position;
            Vector3f cameraInitialFacing = camera.facing;
            float cameraInitialFOV = camera.fov;
            if (keys[GLFW_KEY_W]) { TranslateCamera(GLFW_KEY_W, cameraSpeed);}
            if (keys[GLFW_KEY_S]) {TranslateCamera(GLFW_KEY_S, cameraSpeed);}
            if (keys[GLFW_KEY_A]) {TranslateCamera(GLFW_KEY_A, cameraSpeed);}
            if (keys[GLFW_KEY_D]) {TranslateCamera(GLFW_KEY_D, cameraSpeed);}
            if (keys[GLFW_KEY_UP])    { RotateCameraFacing(GLFW_KEY_UP, rotationSpeed);}
            if (keys[GLFW_KEY_DOWN])  { RotateCameraFacing(GLFW_KEY_DOWN, rotationSpeed);}
            if (keys[GLFW_KEY_LEFT])  { RotateCameraFacing(GLFW_KEY_LEFT, rotationSpeed);}
            if (keys[GLFW_KEY_RIGHT]) { RotateCameraFacing(GLFW_KEY_RIGHT, rotationSpeed);}
            if (keys[GLFW_KEY_SPACE]) {TranslateCamera(GLFW_KEY_SPACE, cameraSpeed*1.5);}
            if (keys[GLFW_KEY_LEFT_CONTROL]) {TranslateCamera(GLFW_KEY_LEFT_CONTROL, cameraSpeed*1.5);}
            if (keys[GLFW_KEY_C]) {
                camera.SetFOVTo(45);
            } else {
                camera.SetFOVTo(120);
            }
        
            return !(cameraInitialFacing == camera.facing) || !(cameraInitialPosition == camera.position) || !(cameraInitialFOV == camera.fov);
        }


};