
#pragma once

#include <GL/glew.h>
#include <iostream>
#include <string>

#define ASSERT(x) if (!(x)) std::abort();
#define GLCALL(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

inline void GLClearError() {
    while(glGetError() != 0){}
}

inline bool GLLogCall(const std::string functionName, const std::string fileName, int line) {
    if(GLenum error = glGetError()) {
        std::cout << "[OpenGL Error] (" << error << ")\n" << \
            "\tin function " << functionName << 
            "\n\tin file " << fileName << ", line " << line << std::endl;
        return false;
    }
    return true;
}