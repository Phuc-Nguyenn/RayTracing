
#pragma once

#include "Renderer.h"

class VertexBuffer {
    private:
        unsigned int m_RendererId;

    public:
        VertexBuffer(const void* data, unsigned int size);
        ~VertexBuffer();

        void Bind() const;
        void Unbind() const;
};

// Implementation templates
VertexBuffer::VertexBuffer(const void* data, unsigned int size) {
    GLCALL(glGenBuffers(1, &m_RendererId));
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_RendererId));
    GLCALL(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

VertexBuffer::~VertexBuffer() {
    GLCALL(glDeleteBuffers(1, &m_RendererId));
}

void VertexBuffer::Bind() const {
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_RendererId));
}

void VertexBuffer::Unbind() const {
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

