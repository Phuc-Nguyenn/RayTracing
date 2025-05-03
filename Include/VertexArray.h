
#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray {
    private:
        unsigned int m_RendererId;

    public:
        VertexArray() {
            GLCALL(glGenVertexArrays(1, &m_RendererId));
        };

        ~VertexArray() {
            GLCALL(glDeleteVertexArrays(1, &m_RendererId));
        };

        void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
            vb.Bind();
            const auto& elements = layout.GetElements();
            unsigned int offset = 0;
            for(unsigned int i = 0; i < elements.size(); i++) {
                const auto& element = elements[i];
                GLCALL(glEnableVertexAttribArray(i));
                GLCALL(glVertexAttribPointer(i, element.count, element.type, element.normalised, layout.GetStride(), (const void*)offset));
                offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
            }
        }

        void Bind() const {
            GLCALL(glBindVertexArray(m_RendererId));
        }

        void Unbind() const {
            GLCALL(glBindVertexArray(0));
        }
};