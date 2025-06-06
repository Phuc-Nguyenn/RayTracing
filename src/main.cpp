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

#define SCREEN_WIDTH 1920   
#define SCREEN_HEIGHT 1080

/* keys state array */
bool keys[1024] = { false };

static Scene CreateScene(unsigned int shaderProgramId) {
    Scene scene(shaderProgramId);

    scene.SetCamera({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 120.0f});

    // Ground plane
    scene.AddObject(std::make_unique<Sphere>(Vector3f{0.0, 0.0, -1000.0}, 1000.0, Material::Lambertian{{0.5, 0.5, 0.5}}));

    for(int i=0; i<=5; ++i) {
        for(int j=0; j<=5; ++j) {
            scene.AddObject(std::make_unique<Sphere>(Vector3f{i*3.0, j*3.0, 1.0}, 1.0, Material::Reflective({1.0, 0.0f, 0.0}, i*0.2, j*0.2)));
        }
    }

    scene.AddObject(std::make_unique<Sphere>(Vector3f{-3.0, -3.0, 1.0}, 1.0, Material::Transparent{{1.0f, 1.0f, 1.0f}, 1.0, 1.33}));

    // Bright Lambertian for ambient lighting effect
    scene.AddObject(std::make_unique<Sphere>(Vector3f{100.0, 100.0, 120.0}, 75, Material::LightSource{{1.0, 1.0, 1.0}}));
    scene.AddObject(std::make_unique<Sphere>(Vector3f{-150.0, 100.0, 100.0}, 80, Material::LightSource{{1.0, 1.0, 1.0}}));

    auto screenResolutionUniformLocation = glGetUniformLocation(shaderProgramId, "screenResolution");
    glUniform2f(screenResolutionUniformLocation, SCREEN_WIDTH, SCREEN_HEIGHT);

    return scene;
}

static void RenderScene(std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window, 
    const std::string& vertexShaderPath, 
    const std::string& fragmentShaderPath,
    const std::string& skyBoxPath
)
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
    GLCALL(glGenTextures(1, &colorBufferTex)); // create a colour buffer texture
    GLCALL(glBindTexture(GL_TEXTURE_2D, colorBufferTex)); // bind the colour buffer texture to GL_TEXTURE_2D
    GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL)); // define the currently bound colour buffer texture
    int accumLocation = glGetUniformLocation(shaderProgramId, "u_Accumulation");
    GLCALL(glUniform1i(accumLocation, 0)); // Bind to texture unit 0

    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0)); // attach the texture to the framebuffer

    GLCALL(glActiveTexture(GL_TEXTURE0));
    GLCALL(glBindTexture(GL_TEXTURE_2D, colorBufferTex));

    Scene scene = CreateScene(shaderProgramId);
    
    /** code for SkyBox  */
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
        "back.png"
    };
    for(unsigned int i = 1; i < textures_faces.size(); i++)
    {
        data = stbi_load((textures_faces[0] + "/" + textures_faces[i]).c_str(), &width, &height, &nrChannels, 0);
        if(data) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i - 1, 
                0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
        } else {
            std::cout << "Failed to load texture at path: " << textures_faces[0] + "/" + textures_faces[i] << std::endl;
        }   
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    GLCALL(glActiveTexture(GL_TEXTURE1)); // texture unit 1
    GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID));
    GLCALL(glUniform1i(glGetUniformLocation(shaderProgramId, "skybox"), 1)); // tell shader: skybox = texture unit 1

    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    auto timePoint = std::chrono::steady_clock::now();
    uint32_t secFrameCount = 0;
    while (!glfwWindowShouldClose(window.get()))
	{
		glfwPollEvents();
        
        bool cameraMoved = scene.HandleCameraMovement(keys);
        if(cameraMoved) {
            scene.ResetFrameIndex();
            GLCALL(glClear(GL_COLOR_BUFFER_BIT));
        }

        scene.Finalise();
        // Render raytrace to framebuffer
        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        GLCALL(glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices)/sizeof(Vector2f)));

        GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        
        GLCALL(glBlitNamedFramebuffer(fbo, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST));

		glfwSwapBuffers(window.get());
        secFrameCount++;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now-timePoint).count() > 1000) {
            timePoint = now;
            uint32_t fps = secFrameCount;
            std::cout << "fps: " << fps << std::endl;
            secFrameCount = 0;
        }
	}
    GLCALL(glDeleteProgram(shaderProgramId));
    glDisableVertexAttribArray(0);
};


static void error_callback(int error, const char* description)
{
	std::cerr << "[GLFW Erro] (" << description << ")" << std::endl;
}

static void key_callback(GLFWwindow* window, const int key, const int, const int action, const int)
{
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <vertex_shader_path> <fragment_shader_path> <skybox_path>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string vertexShaderPath = argv[1];
    std::string fragmentShaderPath = argv[2];
    std::string skyboxPath = argv[3];

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

    RenderScene(std::move(window), vertexShaderPath, fragmentShaderPath, skyboxPath);

    return 0;
}