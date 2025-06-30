#pragma once

#include "Math3D.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <cmath>
#include <array>
#include "Materials.h"
#include "Shape.h" 
#include <cfloat>
#include <limits.h>
#include "BvhTree.h"
#include "ObjectLoader.h"
#include "TextureUnitManager.h"

#define VIEWPORT_DISTANCE 1.0

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
#include <memory>
#include <fstream>     // Correct include for file streams

class Scene {
    private:
        int objectsIndex; // Changed from int to unsigned int
        unsigned int shaderProgramId;
        unsigned int frameIndex;
        Camera camera;

        std::vector<Tri> triangles;
        std::vector<std::unique_ptr<Material::Material>> materials;
        bool inFpsTest;
        float fpsTestAngle;
        uint32_t currentfps;

    public:
        
        Scene(unsigned int shaderProgramId) : shaderProgramId(shaderProgramId), camera{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 90}, frameIndex(0), objectsIndex(0), inFpsTest(false), fpsTestAngle(0) {
            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 4));
        }

        void Finalise() {
            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_FrameIndex"), frameIndex++));
            GLCALL(glUniform3f(glGetUniformLocation(shaderProgramId, "u_RandSeed"), float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX));
        }

        void ResetFrameIndex() {
            frameIndex = 0;
        }

        void SetCurrentFps(uint32_t fps) {
            currentfps = fps;
        }
        
        std::vector<int> FlattenTrianglesMatIdx(const std::vector<Tri>& reorderedTris) {
            std::vector<int> flattened;
            flattened.reserve(reorderedTris.size());
            for(const auto& tri : reorderedTris) {
                flattened.push_back(tri.materialsIndex);
            }
            return flattened;
        }

        std::vector<float> FlattenTrianglesVertices(const std::vector<Tri>& reorderedTris) {
            std::vector<float> flattened;
            flattened.reserve(reorderedTris.size() * 9);
            for (const auto& tri : reorderedTris) {
                // pos1
                flattened.push_back(tri.pos1.x);
                flattened.push_back(tri.pos1.y);
                flattened.push_back(tri.pos1.z);
                // pos2
                flattened.push_back(tri.pos2.x);
                flattened.push_back(tri.pos2.y);
                flattened.push_back(tri.pos2.z);
                // pos3
                flattened.push_back(tri.pos3.x);
                flattened.push_back(tri.pos3.y);
                flattened.push_back(tri.pos3.z);
            }
            return flattened;
        }

        std::vector<float> FlattenBoundingBoxes(const std::vector<BoundingBox>& boundingBoxes) {
            std::vector<float> flattened;
            flattened.reserve(boundingBoxes.size() * 9);
            for (const auto& box : boundingBoxes) {
                // Use 'mini' and 'maxi' as per the BoundingBox definition
                flattened.push_back(box.maxi.x);
                flattened.push_back(box.maxi.y);
                flattened.push_back(box.maxi.z);
                flattened.push_back(box.mini.x);
                flattened.push_back(box.mini.y);
                flattened.push_back(box.mini.z);
                flattened.push_back(static_cast<float>(box.triangleCount));
                if(box.triangleCount == 0) { //if leaf node then
                    flattened.push_back(static_cast<float>(box.rightChildIndex));
                } else {
                    flattened.push_back(static_cast<float>(box.triangleStartIndex));
                }
            }
            return flattened;
        }

        void LoadObjects(const std::vector<std::string>& objectFilePaths) {
            int numberOfFiles = objectFilePaths.size();
            std::unique_ptr<ObjectLoader> objectLoader;
            for(int i=0; i<numberOfFiles; ++i) {
                if(objectFilePaths[i].find(".off") != std::string::npos || objectFilePaths[i].find(".OFF") != std::string::npos) {
                    objectLoader = std::make_unique<OFFLoader>();
                } else {
                    objectLoader = std::make_unique<ObjectLoader>();
                }

                if(!objectLoader->TargetFile(objectFilePaths[i])) {
                    std::cout << "unable to read from: " << objectFilePaths[i] << std::endl;
                    continue;
                }
                std::optional<Material::Material> material = objectLoader->ExtractMaterial();
                if(!material.has_value()) {
                    std::cout << "unable to read material from: " << objectFilePaths[i] << std::endl;
                    continue;
                }
                materials.push_back(std::make_unique<Material::Material>(material.value()));
                std::optional<std::vector<Tri>> objTris = objectLoader->ExtractTriangles();
                if(!objTris.has_value()) {
                    std::cout << "unable to read triangles from: " << objectFilePaths[i] << std::endl;
                    continue;
                }
                triangles.insert(triangles.end(), objTris.value().begin(), objTris.value().end());
            }

            // create the Bvh tree
            BvhTree bvhtree(triangles);
            auto [boundingBoxes, reorderedTriangles] = bvhtree.BuildTree();
            triangles = reorderedTriangles;
            // Flatten all triangles into a single vector
            std::vector<float> trianglesVertexData = FlattenTrianglesVertices(triangles);
            std::vector<int> trianglesMatIdxData = FlattenTrianglesMatIdx(triangles);
            std::vector<float> boundingBoxesData = FlattenBoundingBoxes(boundingBoxes);
            SendDataAsTextureBuffer(trianglesVertexData, triangles.size(), "u_Triangles", TextureUnitManager::getNewTextureUnit(), GL_RGB32F);
            SendDataAsTextureBuffer(trianglesMatIdxData, triangles.size(), "u_MaterialsIndex", TextureUnitManager::getNewTextureUnit(), GL_R32I);
            SendDataAsTextureBuffer(boundingBoxesData, boundingBoxes.size(), "u_BoundingBoxes", TextureUnitManager::getNewTextureUnit(), GL_RGBA32F);
            SendSceneMaterials();
            
            std::cout << "triangles count: " << triangles.size() << std::endl;
            std::cout << "floats count: " << trianglesVertexData.size() << std::endl;
        }

        template<typename T>
        void SendDataAsTextureBuffer(const std::vector<T>& data, const int count, const std::string& uniformName, const int textureUnit, const unsigned int format) {
            if (data.empty()) {
                std::cerr << "Warning: Empty data vector for " << uniformName << std::endl;
                return;
            }
            // create a new buffer and bind the texture buffer to
            GLuint bufferId;
            GLCALL(glGenBuffers(1, &bufferId));
            GLCALL(glBindBuffer(GL_TEXTURE_BUFFER, bufferId));
            GLCALL(glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW));

            GLuint textureId;
            GLCALL(glGenTextures(1, &textureId));
            GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit)); // make texture unit the active unit
            GLCALL(glBindTexture(GL_TEXTURE_BUFFER, textureId)); // associate the texture object with the buffer  
            GLCALL(glTexBuffer(GL_TEXTURE_BUFFER, format, bufferId));

            GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, uniformName.c_str()), textureUnit));
            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, (uniformName + "Count").c_str()), count));
        }

        void SendSceneMaterials() {
            for(int i=0; i<materials.size(); ++i)
            {
                const auto& material = *materials[i];
                auto colourLoc = GetUniformLocationIdx("u_Materials", i, {{"colour"}});
                auto specularcolourLoc = GetUniformLocationIdx("u_Materials", i, {{"specularColour"}});
                auto roughnessLoc = GetUniformLocationIdx("u_Materials", i, {{"roughness"}});
                auto metallicLoc = GetUniformLocationIdx("u_Materials", i, {{"metallic"}});
                auto transparencyLoc = GetUniformLocationIdx("u_Materials", i, {{"transparency"}});
                auto refractionIdxLoc = GetUniformLocationIdx("u_Materials", i, {{"refractionIndex"}});
                auto isLightLoc = GetUniformLocationIdx("u_Materials", i, {{"isLight"}});
                glUniform3f(colourLoc, material.colour.x, material.colour.y, material.colour.z);
                glUniform3f(specularcolourLoc, material.specularColour.x, material.specularColour.y, material.specularColour.z);
                glUniform1f(roughnessLoc, material.roughness);
                glUniform1f(metallicLoc, material.metallic);
                glUniform1f(transparencyLoc, material.transparency);
                glUniform1f(refractionIdxLoc, material.refractionIndex);
                glUniform1i(isLightLoc, material.isLight ? 1 : 0);
            }
            glUniform1i(glGetUniformLocation(shaderProgramId, "u_MaterialsCount"), materials.size()); 
        }

        // void AddShape(const Shape& shape) {
        //     auto typeLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"type"}});
        //     auto positionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position"}});
        //     auto position2Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position2"}});
        //     auto position3Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position3"}});
        //     auto directionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"direction"}});
        //     auto scaleLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"scale"}});
        //     auto colourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"colour"}});
        //     auto specularcolourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"specularColour"}});
        //     auto roughnessLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"roughness"}});
        //     auto metallicLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"metallic"}});
        //     auto transparencyLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"transparency"}});
        //     auto refractionIdxLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"refractionIndex"}});
        //     auto isLightLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"isLight"}});
           
        //     glUniform1ui(typeLoc, shape.getType());
        //     glUniform3f(positionLoc, shape.getPosition().x, shape.getPosition().y, shape.getPosition().z);
        //     glUniform3f(position2Loc, shape.getPosition2().x, shape.getPosition2().y, shape.getPosition2().z);
        //     glUniform3f(position3Loc, shape.getPosition3().x, shape.getPosition3().y, shape.getPosition3().z);
        //     glUniform3f(directionLoc, shape.getDir().x, shape.getDir().y, shape.getDir().z);
        //     glUniform1f(scaleLoc, shape.getScale());
            
        //     const Material::Material material = shape.getMaterial();
        //     glUniform3f(colourLoc, material.colour.x, material.colour.y, material.colour.z);
        //     glUniform3f(specularcolourLoc, material.specularColour.x, material.specularColour.y, material.specularColour.z);
        //     glUniform1f(roughnessLoc, material.roughness);
        //     glUniform1f(metallicLoc, material.metallic);
        //     glUniform1f(transparencyLoc, material.transparency);
        //     glUniform1f(refractionIdxLoc, material.refractionIndex);
        //     glUniform1i(isLightLoc, material.isLight ? 1 : 0);

        //     shapesIndex++;
        // }

        void SetCamera(Camera camera) {
            this->camera = std::move(camera);
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.viewportWidth));
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.viewportDistance));
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.fov"), camera.fov));
        }

        unsigned int GetUniformLocation(std::string uname) {
            std::string query = uname;
            unsigned int result;
            GLCALL(result = glGetUniformLocation(shaderProgramId, query.c_str()));
            return result;
        }

        unsigned int GetUniformLocationIdx(std::string uname, unsigned int index, std::vector<std::string> attributes) {
            std::string indexStr = std::to_string(index);
            std::string query = uname + "[" + indexStr +"]";
            for(auto attribute : attributes) {
                query += "." + attribute;
            }
            unsigned int result;
            GLCALL(result = glGetUniformLocation(shaderProgramId, query.c_str()));
            return result;
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

        Vector3f GetCameraPosition() {
            return camera.position;
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

        std::fstream fpsTestOut; 
        bool HandleCameraMovement(bool keys[]) {
            
            bool refreshStillCamera = false;
            float cameraSpeed = 0.15;
            float rotationSpeed = 0.03;
            if (keys[GLFW_KEY_LEFT_SHIFT]) {
                cameraSpeed = 0.3;
                rotationSpeed = 0.06;
            }
            
            Vector3f cameraInitialPosition = camera.position;
            Vector3f cameraInitialFacing = camera.facing;
            float cameraInitialFOV = camera.fov;

            if(keys[GLFW_KEY_T] && !inFpsTest) { // initiate fps test by turning on the flag and opening the benchmark file
                std::string fpsBenchmarkFile = "fpsBenchmark.csv";
                fpsTestOut.open(fpsBenchmarkFile, std::ios::out);
                if(!fpsTestOut.is_open()) {
                    std::cout << "unable to open file: " << fpsBenchmarkFile << std::endl;

                } else {
                    std::cout << "starting fps test writing to file: " << fpsBenchmarkFile << std::endl;
                    inFpsTest = true;
                }
                fpsTestAngle = 0;
            }
            if(inFpsTest) {
                fpsTestOut << fpsTestAngle << ", " << currentfps << std::endl;

                HandleFpsTestMovement(8);
                if(std::abs(fpsTestAngle - 2*M_PI) < 0.0001) {
                    inFpsTest = false;
                    fpsTestOut.close();
                    std::cout << "finished fps testing" << std::endl;
                }
            } else {
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
                if (keys[GLFW_KEY_GRAVE_ACCENT]) {
                    GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, "u_ViewBoxHits"), 1));
                    refreshStillCamera = true;
                }
                for (unsigned int key = GLFW_KEY_1; key <= GLFW_KEY_6; ++key) {
                    if (keys[key]) {
                        GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, "u_ViewBoxHits"), 0));
                        toggleLowBounce(key);
                        refreshStillCamera = true;
                        break;
                    }
                }
                if (keys[GLFW_KEY_C]) {
                    camera.SetFOVTo(45);
                } else {
                    camera.SetFOVTo(120);
                }
            }
            bool cameraMoved = !(cameraInitialFacing == camera.facing) || !(cameraInitialPosition == camera.position) || !(cameraInitialFOV == camera.fov) || refreshStillCamera;
            if(!(cameraInitialPosition == camera.position) || refreshStillCamera)
                GLCALL(glUniform3f(GetUniformLocation("u_Camera.position"), camera.position.x, camera.position.y, camera.position.z));
            if(!(cameraInitialFacing == camera.facing) || refreshStillCamera)
                GLCALL(glUniform3f(GetUniformLocation("u_Camera.facing"), camera.facing.x, camera.facing.y, camera.facing.z));
            if(!(cameraInitialFOV == camera.fov) || refreshStillCamera) {
                GLCALL(glUniform1f(GetUniformLocation("u_Camera.fov"), camera.fov));
                GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.viewportWidth));
            }
            
            // GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.viewportDistance));

            
            return cameraMoved;
        }
        
        void toggleLowBounce(unsigned int key) { 
            if (key == GLFW_KEY_1) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 1);
            } else if(key == GLFW_KEY_2) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 2);
            } else if(key == GLFW_KEY_3) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 4);
            } else if(key == GLFW_KEY_4) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 8);
            } else if(key == GLFW_KEY_5) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 16);
            } else if(key == GLFW_KEY_6) {
                glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 32);
            }
        };
        unsigned int GetFrameIndex() {
            return frameIndex;
        }
        void HandleFpsTestMovement(float testDistance) {
            camera.position = Vector3f(std::cos(fpsTestAngle), std::sin(fpsTestAngle), 0) * testDistance;
            camera.facing = camera.position.Normalize() * -1;
            fpsTestAngle += M_PI * 0.0015625;
        }
};