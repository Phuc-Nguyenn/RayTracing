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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

/* keys state array */
bool keys[1024] = {false};

static Scene CreateScene(unsigned int shaderProgramId)
{
    Scene scene(shaderProgramId);
    scene.SetCamera({{-8.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 120.0f});
    auto screenResolutionUniformLocation = glGetUniformLocation(shaderProgramId, "screenResolution");
    glUniform2f(screenResolutionUniformLocation, SCREEN_WIDTH, SCREEN_HEIGHT);

    return scene;
}

static void LoadSkybox(unsigned int shaderProgramId, std::string skyBoxPath, unsigned int textureUnit) {
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

    GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit)); // texture unit 1
    GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID));
    GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, "skybox"), 1)); // tell shader: skybox = texture unit 1
}

static void LoadNoiseTexture(unsigned int shaderProgramId, std::string noiseTexturePath, std::string uniformName, unsigned int textureUnit)
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
        GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit)); // activate the texture unit
        GLCALL(glBindTexture(GL_TEXTURE_2D, noiseTextureID));
        GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, uniformName.c_str()), textureUnit)); // tell shader: u_Noise = texture unit
    }
    else
    {
        std::cout << "Failed to load noise texture at path: " << noiseTexturePath << std::endl;
    }
    stbi_image_free(data);
}

static void RenderScene(std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window,
                        const std::string &vertexShaderPath,
                        const std::string &fragmentShaderPath,
                        const std::string &skyBoxPath,
                        const std::vector<std::string>& objectPaths)
{
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

    Scene scene = CreateScene(shaderProgramId);

    /** code for Skybox  */
    LoadSkybox(shaderProgramId, skyBoxPath, 1);

    /** rng noise textures */
    LoadNoiseTexture(shaderProgramId, "./Textures/Noise/rgbSmall.png", "u_RgbNoise", 2);

    scene.LoadObjects(objectPaths);

    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    auto lastFpsCheck = std::chrono::steady_clock::now();
    auto lastPollEvents = std::chrono::steady_clock::now();
    uint32_t secFrameCount = 0;
    uint32_t fps = 0;
    while (!glfwWindowShouldClose(window.get()))
    {
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
        }
        scene.Finalise();
        // Render raytrace to framebuffer
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        GLCALL(glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(Vector2f)));
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCALL(glBlitNamedFramebuffer(fbo, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST));

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