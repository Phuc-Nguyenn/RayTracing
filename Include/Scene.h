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
        unsigned int shapesIndex;
        int objectsIndex; // Changed from int to unsigned int
        unsigned int shaderProgramId;
        unsigned int frameIndex;
        Camera camera;

        std::vector<Tri> triangles;
        std::vector<std::unique_ptr<Material::Material>> materials;

        bool ReadInObjectFile(const std::string verticesFilePath) {
            std::fstream vtxStream(verticesFilePath);
            if (!vtxStream.is_open()) {
                std::cerr << "Failed to open file: " << verticesFilePath << std::endl;
                return false;
            }
            //read in position 
            Vector3f centrePosition = Vector3f(0, 0, 0);
            // should read in material from object file
            std::unique_ptr<Material::Material> material;
            std::string materialType;
            std::string firstArg;
            vtxStream >> firstArg;

            if(firstArg == "position") {
                if(!(vtxStream >> centrePosition.x >> centrePosition.z >> centrePosition.y)) {
                    std::cout << "position argument specified but unable to extract position in " << verticesFilePath << std::endl;
                }
                // Now read the material type after position
                if (!(vtxStream >> materialType)) {
                    std::cout << "unable to read material type after position in " << verticesFilePath << std::endl;
                    return false;
                }
            } else {
                materialType = firstArg;
            }
            bool status = true;

            if(materialType == "default-material") {
                material = std::make_unique<Material::Lambertian>(Vector3f{0.75, 0.75, 0.75});
            } else {
                Vector3f color;
                if (!(vtxStream >> color.x >> color.y >> color.z)) {
                    std::cout << "unable to read material color in " << verticesFilePath << std::endl;
                    return false;
                }
                if(materialType == "specular") {
                    float roughness;
                    status = bool(vtxStream >> roughness);
                    if(status) material = std::make_unique<Material::Specular>(color, roughness); 
                } else if(materialType == "lambertian") {
                    if(status) material = std::make_unique<Material::Lambertian>(color);
                } else if(materialType == "metallic") {
                    float roughness, metallic;
                    status = bool(vtxStream >> roughness >> metallic);
                    if(status) material = std::make_unique<Material::Metallic>(color, roughness, metallic);
                } else if(materialType == "transparent") {
                    float transparency, refractionIndex;
                    status = bool(vtxStream >> transparency >> refractionIndex);
                    if(status) material = std::make_unique<Material::Transparent>(color, transparency, refractionIndex);
                } else if(materialType == "lightsource") {
                    if(status) material = std::make_unique<Material::LightSource>(color);
                } else {
                    std::cout << "unknown material type: " << materialType << std::endl;
                    return false;
                }
            }
            if(!status) {
                std::cout << "unable to read material format" << std::endl;
                return false;
            }
            materials.push_back(std::move(material));
            
            Vector3f x1;
            Vector3f x2;
            Vector3f x3;
            int n;
            vtxStream >> n;
            for(int i=0; i<n; ++i) {
                if(vtxStream >> x1.x >> x1.z >> x1.y >> x2.x >> x2.z >> x2.y >> x3.x >> x3.z >> x3.y) {
                    x1 = x1 + centrePosition; 
                    x2 = x2 + centrePosition; 
                    x3 = x3 + centrePosition;
                    triangles.push_back(Tri(x1, x2, x3, materials.size() - 1));
                } else {
                    std::cout << "failed to read " << verticesFilePath << " on vertex " << i + 1 << std::endl;
                    return false;
                }
            }
            return true;
        };

    public:
        
        Scene(unsigned int shaderProgramId) : shaderProgramId(shaderProgramId), shapesIndex(0), camera{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 90}, frameIndex(0), objectsIndex(0) {
            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 4));
        }

        void Finalise() {
            GLCALL(glUniform3f(GetUniformLocation("u_Camera.position"), camera.position.x, camera.position.y, camera.position.z));
            GLCALL(glUniform3f(GetUniformLocation("u_Camera.facing"), camera.facing.x, camera.facing.y, camera.facing.z));
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.fov"), camera.fov));
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.viewportWidth));
            GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.viewportDistance));

            auto shapeCountLoc = glGetUniformLocation(shaderProgramId, "u_HittablesCount");
            GLCALL(glUniform1ui(shapeCountLoc, shapesIndex));

            // GLCALL(auto objectsCountLoc = glGetUniformLocation(shaderProgramId, "u_ObjectsCount"));
            // GLCALL(glUniform1i(objectsCountLoc, objectsIndex));

            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_FrameIndex"), frameIndex++));
            GLCALL(glUniform3f(glGetUniformLocation(shaderProgramId, "u_RandSeed"), float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX));
        }


        void ResetFrameIndex() {
            frameIndex = 0;
        }


        std::vector<float> FlattenTriangles(const std::vector<Tri>& triangles) {
            std::vector<float> flattened;
            flattened.reserve(triangles.size() * 10);
            for (const auto& tri : triangles) {
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
            // material index
            flattened.push_back(static_cast<float>(tri.materialsIndex));
            }
            return flattened;
        }

        std::vector<float> FlattenBoundingBoxes(const std::vector<BoundingBox>& boundingBoxes) {
            std::vector<float> flattened;
            flattened.reserve(boundingBoxes.size() * (sizeof(BoundingBox)/sizeof(float)));
            for (const auto& box : boundingBoxes) {
            // Use 'mini' and 'maxi' as per the BoundingBox definition
            flattened.push_back(box.mini.x);
            flattened.push_back(box.mini.y);
            flattened.push_back(box.mini.z);
            flattened.push_back(box.maxi.x);
            flattened.push_back(box.maxi.y);
            flattened.push_back(box.maxi.z);
            flattened.push_back(static_cast<float>(box.leftChildIndex));
            flattened.push_back(static_cast<float>(box.rightChildIndex));
            flattened.push_back(static_cast<float>(box.triangleStartIndex));
            flattened.push_back(static_cast<float>(box.triangleCount));
            }
            return flattened;
        }

        void LoadObjects(const std::vector<std::string>& objectFilePaths) {
            int numberOfFiles = objectFilePaths.size();
            for(int i=0; i<numberOfFiles; ++i) {
                if(ReadInObjectFile(objectFilePaths[i])) {
                    std::cout << "successfully read in object from: " << objectFilePaths[i] << std::endl;
                }
            }
            // create the Bvh tree
            BvhTree bvhtree(triangles);
            std::vector<BoundingBox> boundingBoxes = bvhtree.BuildTree();

            // Flatten all triangles into a single vector
            std::vector<float> trianglesData = FlattenTriangles(triangles);
            std::vector<float> boundingBoxesData = FlattenBoundingBoxes(boundingBoxes);
            
            SendDataAsTextureBuffer(trianglesData, triangles.size(), "u_Triangles", 3);
            SendDataAsTextureBuffer(boundingBoxesData, boundingBoxes.size(), "u_BoundingBoxes", 4);

            SendMaterials(materials);
            
            std::cout << "triangles count: " << triangles.size() << std::endl;
            std::cout << "floats count: " << trianglesData.size() << std::endl;
        }

        void SendDataAsTextureBuffer(const std::vector<float>& data, const int count, const std::string& uniformName, const int textureUnit) {
            // Create a buffer texture for triangle data
            GLuint bufferId;
            GLCALL(glGenBuffers(1, &bufferId));
            GLCALL(glBindBuffer(GL_TEXTURE_BUFFER, bufferId));
            GLCALL(glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW));

            // Create the texture buffer object
            GLuint textureId;
            GLCALL(glGenTextures(1, &textureId));
            GLCALL(glBindTexture(GL_TEXTURE_BUFFER, textureId));
            GLCALL(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, bufferId));

            // Bind to texture unit 3
            GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
            GLCALL(glBindTexture(GL_TEXTURE_BUFFER, textureId));

            glUniform1i(glGetUniformLocation(shaderProgramId, uniformName.c_str()), textureUnit);  // '0' corresponds to GL_TEXTURE0
            glUniform1ui(glGetUniformLocation(shaderProgramId, (uniformName + "Count").c_str()), count);
        }

        void SendMaterials(const std::vector<std::unique_ptr<Material::Material>>& materials) {
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

        void AddShape(const Shape& shape) {

            auto typeLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"type"}});
            auto positionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position"}});
            auto position2Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position2"}});
            auto position3Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position3"}});
            auto directionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"direction"}});
            auto scaleLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"scale"}});
            auto colourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"colour"}});
            auto specularcolourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"specularColour"}});
            auto roughnessLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"roughness"}});
            auto metallicLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"metallic"}});
            auto transparencyLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"transparency"}});
            auto refractionIdxLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"refractionIndex"}});
            auto isLightLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"isLight"}});
           
            glUniform1ui(typeLoc, shape.getType());
            glUniform3f(positionLoc, shape.getPosition().x, shape.getPosition().y, shape.getPosition().z);
            glUniform3f(position2Loc, shape.getPosition2().x, shape.getPosition2().y, shape.getPosition2().z);
            glUniform3f(position3Loc, shape.getPosition3().x, shape.getPosition3().y, shape.getPosition3().z);
            glUniform3f(directionLoc, shape.getDir().x, shape.getDir().y, shape.getDir().z);
            glUniform1f(scaleLoc, shape.getScale());
            
            const Material::Material material = shape.getMaterial();
            glUniform3f(colourLoc, material.colour.x, material.colour.y, material.colour.z);
            glUniform3f(specularcolourLoc, material.specularColour.x, material.specularColour.y, material.specularColour.z);
            glUniform1f(roughnessLoc, material.roughness);
            glUniform1f(metallicLoc, material.metallic);
            glUniform1f(transparencyLoc, material.transparency);
            glUniform1f(refractionIdxLoc, material.refractionIndex);
            glUniform1i(isLightLoc, material.isLight ? 1 : 0);

            shapesIndex++;
        }

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
            
            bool refreshStillCamera = false;
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
            for (unsigned int key = GLFW_KEY_1; key <= GLFW_KEY_6; ++key) {
                if (keys[key]) {
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
        
            return !(cameraInitialFacing == camera.facing) || !(cameraInitialPosition == camera.position) || !(cameraInitialFOV == camera.fov) || refreshStillCamera;
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
};