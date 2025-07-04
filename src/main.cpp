#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Math3D.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <cstdlib>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Scene.h"
#include "TextureUnitManager.h"
#include <queue>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

#define TIME_PER_FRAME_MS 5000

/* keys state array */
bool keys[350] = {false};

static Scene CreateScene(std::vector<unsigned int> shaderProgramIds)
{
    auto screenResolutionUniformLocation = glGetUniformLocation(shaderProgramIds[0], "screenResolution");
    glUniform2f(screenResolutionUniformLocation, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    Scene scene(std::move(shaderProgramIds));
    scene.SetCamera({{-8.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 120.0f});
    return scene;
}

static void LoadSkybox(unsigned int shaderProgramId, std::string skyBoxPath) {
    unsigned int skyboxTextureID;
    GLCALL(glGenTextures(1, &skyboxTextureID));
    GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID));

    int width, height, nrChannels;
    unsigned char *data;
    std::vector<std::string> textures_faces = {
        skyBoxPath,
        "right.png",
        "left.png",
        "top.png",
        "bottom.png",
        "front.png",
        "back.png"};
    // Load skybox textures for each face
    for (unsigned int i = 1; i < textures_faces.size(); i++)
    {
        std::string path = textures_faces[0] + "/" + textures_faces[i];
        data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data)
        {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i - 1,
                0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            std::cout << "Failed to load texture at path: " << path << std::endl;
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    GLCALL(glActiveTexture(GL_TEXTURE0 + TextureUnitManager::getNewTextureUnit())); // texture unit 1
    GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID));
    GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, "u_Skybox"), TextureUnitManager::getCurrentTextureUnit())); // tell shader: skybox = texture unit 1
}

static void LoadNoiseTexture(unsigned int shaderProgramId, std::string noiseTexturePath, std::string uniformName)
{
    unsigned int noiseTextureID;
    GLCALL(glGenTextures(1, &noiseTextureID));
    GLCALL(glBindTexture(GL_TEXTURE_2D, noiseTextureID));

    int width, height, nrChannels;
    unsigned char *data = stbi_load(noiseTexturePath.c_str(), &width, &height, &nrChannels, STBI_rgb);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        GLCALL(glActiveTexture(GL_TEXTURE0 + TextureUnitManager::getNewTextureUnit())); // activate the texture unit
        GLCALL(glBindTexture(GL_TEXTURE_2D, noiseTextureID));
        GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, uniformName.c_str()), TextureUnitManager::getCurrentTextureUnit())); // tell shader: u_Noise = texture unit
        GLCALL(glUniform2f(glGetUniformLocation(shaderProgramId, std::string(uniformName + "Resolution").c_str()), width, height));
    }
    else
    {
        std::cout << "Failed to load noise texture at path: " << noiseTexturePath << std::endl;
    }
    stbi_image_free(data);
}

static int imageOutCounter = 0;
static void WriteTexture(unsigned int tmpTexture) {
    int numChannels = 3;
    size_t imageSize = SCREEN_WIDTH * SCREEN_HEIGHT * numChannels;
    std::vector<unsigned char> image(imageSize);
    glBindTexture(GL_TEXTURE_2D, tmpTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    std::string filename = "shots/raytrace-out-" + std::to_string(imageOutCounter++) + ".png";
    stbi_write_png(filename.c_str(), SCREEN_WIDTH, SCREEN_HEIGHT, numChannels, image.data(), SCREEN_WIDTH * numChannels);
    std::cout << "successfully wrote " << filename << std::endl;
    keys[GLFW_KEY_I] = false;
}

static void RenderScene(std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window,
                        const std::string &vertexShaderPath,
                        const std::string &fragmentShaderPath,
                        const std::string &skyBoxPath,
                        const std::vector<std::string>& objectPaths)
{
    TextureUnitManager::ResetTextureUnits();
    
    ShaderProgramSource source = ParseShader(vertexShaderPath, fragmentShaderPath);
    unsigned int shaderProgramId = CreateShaderProgram(source.VertexSource, source.FragmentSource);
    GLCALL(glUseProgram(shaderProgramId));

    Vector2f vertices[6] = {
        Vector2f(1.0, 1.0),
        Vector2f(-1.0, -1.0),
        Vector2f(1.0, -1.0),
        Vector2f(1.0, 1.0),
        Vector2f(-1.0, 1.0),
        Vector2f(-1.0, -1.0),
    };

    VertexArray va;
    va.Bind();
    VertexBuffer vertexBuffer(vertices, sizeof(vertices));
    VertexBufferLayout layout;
    layout.Push<float>(2);
    va.AddBuffer(vertexBuffer, layout);

    unsigned int fbo;
    GLCALL(glGenFramebuffers(1, &fbo));
    GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    // Color texture to accumulate
    unsigned int colorBufferTex;
    GLCALL(glGenTextures(1, &colorBufferTex));  
    GLCALL(glActiveTexture(GL_TEXTURE0));                                                                 // create a colour buffer texture
    GLCALL(glBindTexture(GL_TEXTURE_2D, colorBufferTex));                                                        // bind the colour buffer texture to GL_TEXTURE_2D
    GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL)); // define the currently bound colour buffer texture
    int accumLocation = glGetUniformLocation(shaderProgramId, "u_Accumulation");
    GLCALL(glUniform1i(accumLocation, 0)); // Bind to texture unit 0

    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0)); // attach the texture to the framebuffer

    // bloom sources and additional shaders
    ShaderProgramSource finalProgramSource = ParseShader("./src/shaders/finalVertex.glsl", "./src/shaders/finalFragment.glsl");
    unsigned int finalProgramId = CreateShaderProgram(finalProgramSource.VertexSource, finalProgramSource.FragmentSource);
    GLCALL(glUseProgram(finalProgramId));
    GLCALL(glUniform1i(glGetUniformLocation(finalProgramId, "u_Original"), 0));
    GLCALL(GLCALL(glUniform2f(glGetUniformLocation(finalProgramId, "u_Resolution"), SCREEN_WIDTH, SCREEN_HEIGHT)));

    ShaderProgramSource extractBrightnessSource = ParseShader("./src/shaders/extractBrightVertex.glsl", "./src/shaders/extractBrightFragment.glsl");
    unsigned int thresholdProgramId = CreateShaderProgram(extractBrightnessSource.VertexSource, extractBrightnessSource.FragmentSource);

    ShaderProgramSource bloomSource = ParseShader("./src/shaders/bloomVertex.glsl", "./src/shaders/bloomFragment.glsl");
    unsigned int bloomProgramId = CreateShaderProgram(bloomSource.VertexSource, bloomSource.FragmentSource);


    GLCALL(glUseProgram(shaderProgramId));
    Scene scene = CreateScene({shaderProgramId, finalProgramId});
    /** code for Skybox  */
    LoadSkybox(shaderProgramId, skyBoxPath);
    /** rng noise textures */
    LoadNoiseTexture(shaderProgramId, "./Textures/Noise/rgbNoiseSquare.png", "u_RgbNoise");

    scene.LoadObjects(objectPaths);


    int bufferTextureCount = 4;
    std::vector<std::vector<unsigned int>> bufferTextures(bufferTextureCount, std::vector<unsigned int>(3)); // <bufferId><textureId><textureUnit>
    std::vector<int> downSamplingAmounts = {10, 20, 40, 80}; // must be the same count as buffer texture count
    ASSERT(downSamplingAmounts.size() == bufferTextureCount);

    // Create and bind a second (larger) framebuffer for denoising if not already created
    glUseProgram(finalProgramId);
    for(int i=0; i<bufferTextureCount; ++i) {
        bufferTextures[i][2] = TextureUnitManager::getNewTextureUnit();
        GLCALL(glUniform1i(glGetUniformLocation(finalProgramId, std::string("u_Bloom" + std::to_string(i)).c_str()), bufferTextures[i][2]));

        GLCALL(glGenFramebuffers(1, &bufferTextures[i][0])); // create the frame buffer object
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, bufferTextures[i][0])); // bind it to the GL_FRAMEBUFFER target
        GLCALL(glGenTextures(1, &bufferTextures[i][1])); // generate the texture
        GLCALL(glActiveTexture(GL_TEXTURE0 + bufferTextures[i][2]));
        GLCALL(glBindTexture(GL_TEXTURE_2D, bufferTextures[i][1])); // bind it to the GL_TEXTURE_2D target
        GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)); 
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferTextures[i][1], 0)); // make the texture the color attachment of the framebuffer
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0)); // unbind frame buffer
    }

    unsigned int tmpBuffer, tmpTexture, tmpTextureId;
    tmpTextureId = TextureUnitManager::getNewTextureUnit();
    GLCALL(glGenFramebuffers(1, &tmpBuffer));
    GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, tmpBuffer));
    GLCALL(glGenTextures(1, &tmpTexture));
    GLCALL(glActiveTexture(GL_TEXTURE0 + tmpTextureId));
    GLCALL(glBindTexture(GL_TEXTURE_2D, tmpTexture)); // bind it to the GL_TEXTURE_2D target
    GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)); 
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmpTexture, 0)); // make the texture the color attachment of the framebuffer
    GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // loop initialisation logic
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    auto lastFpsCheck = std::chrono::steady_clock::now();
    auto lastPollEvents = std::chrono::steady_clock::now();
    auto lastCapturedFrame = std::chrono::steady_clock::now();
    uint32_t secFrameCount = 0;
    uint32_t fps = 0;
    int imageOutCounter = 0;
    bool isRecordingCamPos = false;
    bool isPlayingBack = false;
    std::queue<std::pair<Vector3f, Vector3f>> RecordCamData;
    while (!glfwWindowShouldClose(window.get()))
    {
        GLCALL(glUseProgram(shaderProgramId));
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsCheck).count() > 1000)
        {
            lastFpsCheck = now;
            fps = secFrameCount;
            Vector3f cameraPosition = scene.GetCameraPosition();
            std::cout << "fps: " << fps << ", total_frames: " << scene.GetFrameIndex();
            std::cout << ",\t camera position: " << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << std::endl;
            secFrameCount = 0;
        }
        if(isPlayingBack && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCapturedFrame).count() > TIME_PER_FRAME_MS) {
            lastCapturedFrame = now;
            WriteTexture(tmpTexture);
            if(RecordCamData.empty()) {
                isPlayingBack = false;
            } else {
                Vector3f nextpos = RecordCamData.front().first;
                Vector3f nextdir = RecordCamData.front().second;
                RecordCamData.pop();
                scene.SetCamera(Camera(nextpos, nextdir, scene.camera.fov));
                scene.ResetFrameIndex();
                GLCALL(glClear(GL_COLOR_BUFFER_BIT));
            }
        }
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPollEvents).count() > 10)
        {
            lastPollEvents = now;
            glfwPollEvents();
            scene.SetCurrentFps(fps);
            bool cameraMoved = scene.HandleCameraMovement(keys);
            if (cameraMoved)
            {
                scene.ResetFrameIndex();
                GLCALL(glClear(GL_COLOR_BUFFER_BIT));
            }
            if(keys[GLFW_KEY_R]) {
                if(isRecordingCamPos) {
                    std::cout << "stopped recording" << std::endl;
                    isRecordingCamPos = false;
                } else {
                    std::cout << "started recording" << std::endl;
                    RecordCamData = {};
                    isRecordingCamPos = true;
                }
                keys[GLFW_KEY_R] = false;
            }
            if(isRecordingCamPos) {
                RecordCamData.push({scene.camera.position, scene.camera.facing});
            }
            if(keys[GLFW_KEY_P]) {
                isPlayingBack = true;
                keys[GLFW_KEY_P] = false;
            }
        }
        scene.Finalise();
        // Render raytrace to framebuffer
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        GLCALL(glDrawArrays(GL_TRIANGLES, 0, 6));

        if(keys[GLFW_KEY_B] == true || scene.GetInBoxHitView()) {
            GLCALL(glBlitNamedFramebuffer(
                fbo, 0,
                0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                GL_COLOR_BUFFER_BIT, GL_NEAREST));
        } else {
            for(int i=0; i<bufferTextureCount; i++) {
                // Blit the rendered image to the tmp framebuffer
                GLCALL(glBlitNamedFramebuffer(
                    fbo, tmpBuffer,
                    0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    0, 0, SCREEN_WIDTH/downSamplingAmounts.at(i), SCREEN_HEIGHT/downSamplingAmounts.at(i),
                    GL_COLOR_BUFFER_BIT, GL_NEAREST));

                GLCALL(glUseProgram(thresholdProgramId)); // get the bright parts of the screen using the threshold program and draw onto the images buffer texture
                GLCALL(glUniform1i(glGetUniformLocation(thresholdProgramId, "u_Image"), tmpTextureId));
                GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, bufferTextures[i][0]));
                GLCALL(glDrawArrays(GL_TRIANGLES, 0, 6));
                
                GLCALL(glUseProgram(bloomProgramId)); // apply bloom to the buffer texture and place it back into tmp
                GLCALL(glUniform1i(glGetUniformLocation(bloomProgramId, "u_Image"), bufferTextures[i][2]));
                GLCALL(glUniform2f(glGetUniformLocation(bloomProgramId, "u_ImageResolution"), SCREEN_WIDTH/downSamplingAmounts.at(i), SCREEN_HEIGHT/downSamplingAmounts.at(i)));
                GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, tmpBuffer)); 
                GLCALL(glDrawArrays(GL_TRIANGLES, 0, 6));

                GLCALL(glBlitNamedFramebuffer( // upscale the bloomed brightness regions to buffer texture again to be used in the final additive blending program
                    tmpBuffer, bufferTextures[i][0],
                    0, 0, SCREEN_WIDTH/downSamplingAmounts.at(i), SCREEN_HEIGHT/downSamplingAmounts.at(i),
                    0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR));
            }

            glUseProgram(finalProgramId);
            glBindFramebuffer(GL_FRAMEBUFFER, tmpBuffer);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            GLCALL(glBlitNamedFramebuffer( // upscale the bloomed brightness regions to buffer texture again to be used in the final additive blending program
                    tmpBuffer, 0,
                    0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST));
        }   

        glfwSwapBuffers(window.get());
        secFrameCount++;
    }
    GLCALL(glDeleteProgram(shaderProgramId));
    glDisableVertexAttribArray(0);
};

static void error_callback(int error, const char *description)
{
    std::cerr << "[GLFW Erro] (" << description << ")" << std::endl;
}

static void key_callback(GLFWwindow *window, const int key, const int, const int action, const int)
{
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <absolute_path_prefix> <rel_vertex_shader_path> <rel_fragment_shader_path> <rel_skybox_path> [<object_path_1>]" << std::endl;
        return EXIT_FAILURE;
    }
    std::string vertexShaderPath = argv[1];
    std::string fragmentShaderPath = argv[2];
    std::string skyboxPath = argv[3];
    std::vector<std::string> objectPaths;
    for(int i=4; i < argc; ++i) {
        objectPaths.push_back(argv[i]);
    }

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
        glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL), glfwDestroyWindow);
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window.get());

    glfwSwapInterval(1);
    glfwSetKeyCallback(window.get(), key_callback);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "[GLEW Error] (" << glewGetErrorString(err) << ")" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    RenderScene(std::move(window), vertexShaderPath, fragmentShaderPath, skyboxPath, objectPaths);

    return 0;
}