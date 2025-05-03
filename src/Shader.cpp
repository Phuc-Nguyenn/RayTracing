
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include "Shader.h"
#include "Renderer.h"

ShaderProgramSource ParseShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    std::ifstream vertexShaderStream(vertexShaderPath);
    std::ifstream fragmentShaderStream(fragmentShaderPath);
    std::string line;
    std::stringstream ss[2];
    while(getline(vertexShaderStream, line)) {
            ss[0] << line << '\n';
    }
    while(getline(fragmentShaderStream, line)) {
        ss[1] << line << '\n';
    }
    return {ss[0].str(), ss[1].str()};
}

unsigned int CompileShader(unsigned int type, const std::string& source) {
    GLCALL(unsigned int id = glCreateShader(type));
    const char* src = source.c_str();
    GLCALL(glShaderSource(id, 1, &src, nullptr));
    GLCALL(glCompileShader(id));
    
    /** error handling */
    int result;
    GLCALL(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if(result == GL_FALSE) {
        int length;
        GLCALL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        char* message = (char*)alloca(length * sizeof(char));
        GLCALL(glGetShaderInfoLog(id, length, &length, message));
        std::cout << "Failed to compile shader of type " << 
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << std::endl;
        std::cout << message << std::endl;
        GLCALL(glDeleteShader(id));
        return 0;
    }
    
    return id;
}

unsigned int CreateShaderProgram(const std::string& vertexShader, const std::string& fragmentShader) {
    GLCALL(unsigned int program = glCreateProgram());
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    GLCALL(glAttachShader(program, fs));
    GLCALL(glAttachShader(program, vs));
    GLCALL(glLinkProgram(program));
    GLCALL(glValidateProgram(program));
    GLCALL(glDeleteShader(fs));
    GLCALL(glDeleteShader(vs));
    return program;  
}