#pragma once

#include "Camera.h"
#include "Scene.h"
#include "KeyEventObserver.h"
#include "Utils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>
#include <queue>


// thin wrapper around camera for it to record frames
class Recorder {
enum State {
    NOTHING,
    RECORDING,
    PLAYBACK,

    STATES_COUNT
};

public:
    Recorder(Camera& camera, GLuint texture, unsigned int textureWidth, unsigned int textureHeight) :
        state(NOTHING), 
        keyEventObserver([this](const KeyEvent& event) {this->OnEvent(event);}),
        lastPlaybackTime(std::chrono::steady_clock::now()),
        lastRecordTime(std::chrono::steady_clock::now()),
        camera(camera),
        texture(texture),
        textureWidth(textureWidth),
        textureHeight(textureHeight)
    {}

    void Tick() {
        // record cam position every 10 ms
        if(state == RECORDING && Utils::TimeSinceMs(lastRecordTime) > recordIntervalMs) {
            lastRecordTime = std::chrono::steady_clock::now();
            recordData.push({camera.GetPosition(), camera.GetFacing()});
        }
        
        // change playback camera position every 5 seconds
        if(state == PLAYBACK &&  Utils::TimeSinceMs(lastPlaybackTime) > playbackIntervalMs) {
            lastPlaybackTime = std::chrono::steady_clock::now();
            WriteTexture();
            Vector3f nextpos = recordData.front().first;
            Vector3f nextdir = recordData.front().second;
            recordData.pop();
            camera.SetPosition(nextpos);
            camera.SetFacing(nextdir);
            camera.UploadInfo();
            camera.GetScene().ResetFrameIndex();
        }
    }

private:

    std::queue<std::pair<Vector3f, Vector3f>> recordData;
    State state;
    KeyEventObserver keyEventObserver;
    Camera& camera;
    GLuint texture; 
    unsigned int textureWidth;
    unsigned int textureHeight;
    unsigned imageOutCounter = 0;

    std::chrono::steady_clock::time_point lastPlaybackTime;
    std::chrono::steady_clock::time_point lastRecordTime;
    const unsigned int playbackIntervalMs = 5000;
    const unsigned int recordIntervalMs = 10;

    void OnEvent(const KeyEvent& event) {
        if(event.action != GLFW_PRESS) {
            return;
        }
        if(event.key == GLFW_KEY_R) {
            if(state == NOTHING) {
                std::cout << "started recording" << std::endl;
                state = RECORDING;
            } else if (state == RECORDING) {
                std::cout << "stopped recording" << std::endl;
                state = NOTHING;
            }
            return;
        }
        if(event.key == GLFW_KEY_P && state == NOTHING) {
            std::cout << "started playback captures" << std::endl; 
            state = PLAYBACK;
        }
    };

    void WriteTexture() {
        int numChannels = 3;
        size_t imageSize = textureWidth * textureHeight * numChannels;
        std::vector<unsigned char> image(imageSize);
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
        std::string filename = "shots/raytrace-out-" + std::to_string(imageOutCounter++) + ".png";
        stbi_write_png(filename.c_str(), textureWidth, textureHeight, numChannels, image.data(), textureWidth * numChannels);
        std::cout << "successfully wrote " << filename << std::endl;
        std::cout << "captures left to render: " << recordData.size() << std::endl;
    }
};