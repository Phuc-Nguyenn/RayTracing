#pragma once
#include <string>

struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

ShaderProgramSource ParseShader(const std::string& , const std::string&);

unsigned int CompileShader(unsigned int type, const std::string& source);

unsigned int CreateShaderProgram(const std::string& vertexShader, const std::string& fragmentShader);