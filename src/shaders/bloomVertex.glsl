#version 430 core

in vec4 position;
out vec4 gl_Position;

void main()
{
    gl_Position = position;
}