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
        std::vector<std::vector<Triangle>> objects;
        int trianglesCount;

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

            std::string clusterOption;
            int numberOfClusters, n = 0;
            vtxStream >> clusterOption;
            if(clusterOption != "clusters") {
                std::cout << "unable to read number of clusters, defualt to sqrt(n)" << std::endl;
                n = std::stoi(clusterOption); // actually n not number of clusters
                numberOfClusters = sqrt(n);
            } else {
                // read in clusters then number of triangles
                vtxStream >> numberOfClusters >> n;
            }
            
            Vector3f x1;
            Vector3f x2;
            Vector3f x3;
            std::vector<std::pair<Triangle, int>> clusters;
            Vector3f maxCoords = Vector3f(INT_MIN,INT_MIN,INT_MIN);
            Vector3f minCoords = Vector3f(INT_MAX,INT_MAX,INT_MAX);
            for(int i=0; i<n; ++i) {
                if(vtxStream >> x1.x >> x1.z >> x1.y >> x2.x >> x2.z >> x2.y >> x3.x >> x3.z >> x3.y) {
                    x1 = x1 + centrePosition; x2 = x2 + centrePosition; x3 = x3 + centrePosition;
                    clusters.push_back({{x1, x2, x3, *material, false}, -1});
                    maxCoords.x = std::max(maxCoords.x, std::max(x1.x, std::max(x2.x, x3.x)));
                    maxCoords.y = std::max(maxCoords.y, std::max(x1.y, std::max(x2.y, x3.y)));
                    maxCoords.z = std::max(maxCoords.z, std::max(x1.z, std::max(x2.z, x3.z)));
                    minCoords.x = std::min(minCoords.x, std::min(x1.x, std::min(x2.x, x3.x)));
                    minCoords.y = std::min(minCoords.y, std::min(x1.y, std::min(x2.y, x3.y)));
                    minCoords.z = std::min(minCoords.z, std::min(x1.z, std::min(x2.z, x3.z)));
                } else {
                    std::cout << "failed to read " << verticesFilePath << " on vertex " << i + 1 << std::endl;
                    return false;
                }
            }
            Vector3f maxMinDiff = maxCoords - minCoords;

            std::vector<Vector3f> meanCenters;
            for(int k=0; k<numberOfClusters; ++k) { // initialize mean centers
                meanCenters.push_back({float(std::rand())/RAND_MAX, float(std::rand())/RAND_MAX, float(std::rand())/RAND_MAX});
                meanCenters.back() = maxMinDiff * meanCenters.back() + minCoords;
            }

            std::vector<std::pair<Vector3f, int>> means(numberOfClusters, {Vector3f(0,0,0), 0});
            int convergenceIterations = 64; // default

            for(int i=0; i<convergenceIterations; ++i) {
                // for each triangle determine which mean it should belong too
                for(auto& [triangle, matchingcluster] : clusters) {
                    float minDistance = FLT_MAX;
                    int minCluster = -1;
                    for(int j=0; j<meanCenters.size(); ++j) {
                        //distance
                        float distanceToMean = std::max((triangle.getPosition() - meanCenters[j]).len(), 
                                                std::max((triangle.getPosition2() - meanCenters[j]).len(),
                                                        (triangle.getPosition3() - meanCenters[j]).len()));
                        if(distanceToMean < minDistance) {
                            minDistance = distanceToMean;
                            minCluster = j;
                        }
                    }
                    matchingcluster = minCluster;
                    means[minCluster].first = means[minCluster].first + triangle.GetAveragePosition();
                    means[minCluster].second += 1;
                }

                // calculate the means and update the cluster centers
                for(int k=0; k<numberOfClusters; ++k) {
                    Vector3f newCenter = (means[k].second > 0) ? (means[k].first / means[k].second) : meanCenters[k];
                    meanCenters[k] = newCenter;
                    means[k] = {Vector3f(0,0,0), 0};
                    // std::cout << "iteration " << i << ", cluster " << k << ": " << newCenter.x << ", " << newCenter.y << ", " << newCenter.z << std::endl;
                }
                // std::cout << std::endl;
            }

            // now we must demultiplex them and add to vertices in order
            std::vector<std::vector<Triangle>> demultiplexedClusters(numberOfClusters);
            for(const auto& [triangle, matchingCluster] : clusters) {
                demultiplexedClusters[matchingCluster].push_back(triangle);
            }
            std::vector<float> clusterRadii(numberOfClusters, 0);
            int clusterIndex = 0;
            for(const auto& cluster : demultiplexedClusters) {
                if(cluster.size() == 0) {
                    clusterIndex++;
                    continue;
                }
                objects.emplace_back();
                auto& vertices = objects.back();
                vertices.reserve(cluster.size());
                float radius = 0;
                for(const auto& triangle : cluster) {
                    float distanceToMean = std::max((triangle.getPosition() - meanCenters[clusterIndex]).len(), 
                                            std::max((triangle.getPosition2() - meanCenters[clusterIndex]).len(),
                                                    (triangle.getPosition3() - meanCenters[clusterIndex]).len()));
                    radius = std::max(radius, distanceToMean);
                    vertices.push_back(triangle);
                }
                clusterRadii[clusterIndex] = radius;
                clusterIndex++;
            }


            // each cluster is its own object so add numberOfClusters amount of new objects 
            for(int i=0; i<numberOfClusters; ++i) {
                if(demultiplexedClusters[i].size() == 0) {
                    continue;
                }
                int prevTrianglesCount = trianglesCount;
                trianglesCount += demultiplexedClusters[i].size();
                auto positionLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"position"}});
                auto scaleLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"scale"}});
                auto trianglesStartIndexLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"trianglesStartIndex"}});
                auto trianglesCountLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"trianglesCount"}});
                auto colourLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"colour"}});
                auto specularcolourLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"specularColour"}});
                auto roughnessLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"roughness"}});
                auto metallicLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"metallic"}});
                auto transparencyLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"transparency"}});
                auto refractionIdxLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"refractionIndex"}});
                auto isLightLoc = GetUniformLocationIdx("u_Objects", objectsIndex, {{"material"}, {"isLight"}});
                
                std::cout << "object index: " << objectsIndex << ", start index: " << prevTrianglesCount << ", cluster triangle count: " << trianglesCount - prevTrianglesCount << std::endl;
                GLCALL(glUniform3f(positionLoc, meanCenters[i].x, meanCenters[i].y, meanCenters[i].z));
                GLCALL(glUniform1f(scaleLoc, clusterRadii[i]+0.01));
                GLCALL(glUniform1i(trianglesStartIndexLoc, prevTrianglesCount));
                GLCALL(glUniform1i(trianglesCountLoc, trianglesCount - prevTrianglesCount));
                GLCALL(glUniform3f(colourLoc, material->colour.x, material->colour.y, material->colour.z));
                GLCALL(glUniform3f(specularcolourLoc, material->specularColour.x, material->specularColour.y, material->specularColour.z));
                GLCALL(glUniform1f(roughnessLoc, material->roughness));
                GLCALL(glUniform1f(metallicLoc, material->metallic));
                GLCALL(glUniform1f(transparencyLoc, material->transparency));
                GLCALL(glUniform1f(refractionIdxLoc, material->refractionIndex));
                GLCALL(glUniform1i(isLightLoc, material->isLight ? 1 : 0));
                objectsIndex++;
            }
            
            return true;
        };
        
        void FlattenObjectsToImage(std::vector<float>& allTriangles) {
            for(const auto& object : objects) {
                for(const auto& triangle : object) {
                    // Position 1
                    allTriangles.push_back(triangle.getPosition().x);
                    allTriangles.push_back(triangle.getPosition().y);
                    allTriangles.push_back(triangle.getPosition().z);
                    
                    // Position 2
                    allTriangles.push_back(triangle.getPosition2().x);
                    allTriangles.push_back(triangle.getPosition2().y);
                    allTriangles.push_back(triangle.getPosition2().z);
                    
                    // Position 3
                    allTriangles.push_back(triangle.getPosition3().x);
                    allTriangles.push_back(triangle.getPosition3().y);
                    allTriangles.push_back(triangle.getPosition3().z);
                }
            }
        }


    public:
        
        Scene(unsigned int shaderProgramId) : shaderProgramId(shaderProgramId), shapesIndex(0), camera{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 90}, frameIndex(0), trianglesCount(0), objectsIndex(0) {
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

            GLCALL(auto objectsCountLoc = glGetUniformLocation(shaderProgramId, "u_ObjectsCount"));
            GLCALL(glUniform1i(objectsCountLoc, objectsIndex));

            GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_FrameIndex"), frameIndex++));
            GLCALL(glUniform3f(glGetUniformLocation(shaderProgramId, "u_RandSeed"), float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX));
        }


        void ResetFrameIndex() {
            frameIndex = 0;
        }

        void LoadObjects(std::vector<std::string> objectFilePaths) {
            int numberOfFiles = objectFilePaths.size();
            for(int i=0; i<numberOfFiles; ++i) {
                if(ReadInObjectFile(objectFilePaths[i])) {
                    std::cout << "successfully read in object from: " << objectFilePaths[i] << std::endl;
                }
            }

            // Flatten all triangles into a single vector
            std::vector<float> objectsData;
            FlattenObjectsToImage(objectsData);
            
            // Create a buffer texture for triangle data
            GLuint triangleBufferId;
            GLCALL(glGenBuffers(1, &triangleBufferId));
            GLCALL(glBindBuffer(GL_TEXTURE_BUFFER, triangleBufferId));
            GLCALL(glBufferData(GL_TEXTURE_BUFFER, objectsData.size() * sizeof(float), objectsData.data(), GL_STATIC_DRAW));

            // Create the texture buffer object
            GLuint triangleTextureId;
            GLCALL(glGenTextures(1, &triangleTextureId));
            GLCALL(glBindTexture(GL_TEXTURE_BUFFER, triangleTextureId));
            GLCALL(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, triangleBufferId));

            // Bind to texture unit 3
            GLCALL(glActiveTexture(GL_TEXTURE3));
            GLCALL(glBindTexture(GL_TEXTURE_BUFFER, triangleTextureId));

            glUniform1i(glGetUniformLocation(shaderProgramId, "u_Triangles"), 3);  // '0' corresponds to GL_TEXTURE0
            glUniform1ui(glGetUniformLocation(shaderProgramId, "u_TrianglesCount"), trianglesCount);
            
            std::cout << "objects count: " << objects.size() << std::endl;
            std::cout << "triangles count: " << trianglesCount << std::endl;
            std::cout << "floats count: " << objectsData.size() << std::endl;
        }


        void AddShape(const Shape& shape) {

            auto typeLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"type"}});
            auto positionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position"}});
            auto position2Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position2"}});
            auto position3Loc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"position3"}});
            auto directionLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"direction"}});
            auto scaleLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"scale"}});
            auto colourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"colour"}});
            auto specularcolourLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"specularColour"}});
            auto roughnessLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"roughness"}});
            auto metallicLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"metallic"}});
            auto transparencyLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"transparency"}});
            auto refractionIdxLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"refractionIndex"}});
            auto isLightLoc = GetUniformLocationIdx("u_Hittable", shapesIndex, {{"material"}, {"isLight"}});
           
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