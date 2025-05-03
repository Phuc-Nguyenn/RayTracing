
#pragma once

#include "Renderer.h"

class IndexBuffer {
    private:
        unsigned int m_RendererId;
        unsigned int m_Count;

    public:
        IndexBuffer(const void* data, unsigned int count);
        ~IndexBuffer();

        void Bind() const;
        void Unbind() const;

        unsigned int GetCount();
};

// Implementation templates
IndexBuffer::IndexBuffer(const void* data, unsigned int count) : m_Count(count) {
    GLCALL(glGenBuffers(1, &m_RendererId));
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId));
    GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

IndexBuffer::~IndexBuffer() {
    GLCALL(glDeleteBuffers(1, &m_RendererId));
}

void IndexBuffer::Bind() const {
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId));
}

void IndexBuffer::Unbind() const {
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

unsigned int IndexBuffer::GetCount() {
    return m_Count;
}

