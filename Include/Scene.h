#pragma once

#include "Math3D.h"
#include "Materials.h"
#include "Tri.h" 
#include "BvhTree.h"
#include "ObjectLoader.h"
#include "TextureUnitManager.h"
#include "Camera.h"
#include "Renderer.h"
#include "BounceLimitManager.h"

#include <iostream>
#include <memory>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cfloat>
#include <limits.h>

#define TRACER_ID 0

class Scene {
    public:
        Scene(std::vector<unsigned int> shaderProgramIds);
        
        void ResetFrameIndex() { frameIndex = 0; }
        
        unsigned int GetFrameIndex() const { return frameIndex; }
        
        void Tick();
        
        void SetCurrentFps(uint32_t fps) { currentfps = fps; }
        
        bool GetInBoxHitView() const { return inBoxHitView; }
        
        void LoadObjects(const std::vector<std::string>& objectFilePaths);
        
        const Camera& GetCamera() const { return camera; };
        
        Camera& GetCamera() { return camera; };

        const std::vector<unsigned int>& GetShaderProgramIds() { return shaderProgramIds; };

    private:
        int objectsIndex; // Changed from int to unsigned int
        unsigned int shaderProgramId;
        unsigned int frameIndex;
        BounceLimitManager bounceLimitManager;
        Camera camera;

        std::vector<Tri> triangles;
        std::vector<std::unique_ptr<Material::Material>> materials;
        bool inFpsTest;
        float fpsTestAngle;
        uint32_t currentfps;
        bool inBoxHitView;
        std::vector<unsigned int> shaderProgramIds;
        float denoiseOptionValues[3] = {1.0f, 1.0f, 0.05f};
        std::fstream fpsTestOut; 

        std::vector<int> FlattenTrianglesMatIdx(const std::vector<Tri>& reorderedTris);

        std::vector<float> FlattenTrianglesVertices(const std::vector<Tri>& reorderedTris);

        std::vector<float> FlattenBoundingBoxes(const std::vector<BoundingBox>& boundingBoxes);
        
        template<typename T>
        void SendDataAsSSBO(const std::vector<T>& data, const int bufferUnit, const GLenum usageType) {
            if(data.empty()) {
                std::cerr << " Warning: Empty data vector for buffer unit " << bufferUnit << std::endl;
                return;
            }

            GLuint ssbo;
            glGenBuffers(1, &ssbo);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(), usageType);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bufferUnit, ssbo);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
            glBindBuffer(GL_TEXTURE_BUFFER, 0);
        }

        void SendSceneMaterials();

        unsigned int GetUniformLocation(std::string uname);

        unsigned int GetUniformLocationIdx(std::string uname, unsigned int index, std::vector<std::string> attributes);
};