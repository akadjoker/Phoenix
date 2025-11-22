#include "pch.h"
#include "Vertex.hpp"

u32 CalculatePrimitiveCount(PrimitiveType type, u32 vertexCount)
{
    switch (type)
    {
    case PT_POINTS:
        return vertexCount;
    case PT_LINES:
        return vertexCount / 2;
    case PT_LINE_STRIP:
        return vertexCount > 1 ? vertexCount - 1 : 0;
    case PT_LINE_LOOP:
        return vertexCount;
    case PT_TRIANGLES:
        return vertexCount / 3;
    case PT_TRIANGLE_STRIP:
    case PT_TRIANGLE_FAN:
        return vertexCount > 2 ? vertexCount - 2 : 0;
    default:
        return 0;
    }
}

bool ValidateVertexCount(PrimitiveType type, u32 vertexCount)
{
    switch (type)
    {
    case PT_POINTS:
        return vertexCount >= 1;
    case PT_LINES:
        return vertexCount >= 2 && (vertexCount % 2 == 0);
    case PT_LINE_STRIP:
    case PT_LINE_LOOP:
        return vertexCount >= 2;
    case PT_TRIANGLES:
        return vertexCount >= 3 && (vertexCount % 3 == 0);
    case PT_TRIANGLE_STRIP:
    case PT_TRIANGLE_FAN:
        return vertexCount >= 3;
    default:
        return false;
    }
}

// ============================================================================
// VERTEX ELEMENT
// ============================================================================

VertexElement::VertexElement()
    : stream(0), offset(0), type(VET_FLOAT1), semantic(VES_POSITION), index(0), instanceDivisor(0)
{
}

u32 VertexElement::GetSize() const
{
    static const u32 sizes[] = {
        sizeof(float),     // VET_FLOAT1
        sizeof(float) * 2, // VET_FLOAT2
        sizeof(float) * 3, // VET_FLOAT3
        sizeof(float) * 4, // VET_FLOAT4
        4,                 // VET_COLOR
        sizeof(short) * 2, // VET_SHORT2
        sizeof(short) * 4, // VET_SHORT4
        4                  // VET_UBYTE4
    };
    return sizes[type];
}

u32 VertexElement::GetComponentCount() const
{
    static const u32 counts[] = {1, 2, 3, 4, 4, 2, 4, 4};
    return counts[type];
}

u32 VertexElement::GetType() const
{
    switch (type)
    {
    case VET_FLOAT1:
    case VET_FLOAT2:
    case VET_FLOAT3:
    case VET_FLOAT4:
        return GL_FLOAT;
    case VET_COLOR:
    case VET_UBYTE4:
        return GL_UNSIGNED_BYTE;
    case VET_SHORT2:
    case VET_SHORT4:
        return GL_SHORT;
    default:
        return GL_FLOAT;
    }
}

bool VertexElement::ShouldNormalize() const
{
    return type == VET_COLOR || type == VET_UBYTE4;
}

// ============================================================================
// VERTEX DECLARATION
// ============================================================================

VertexDeclaration::VertexDeclaration()
    : m_isDirty(true)
{
}

void VertexDeclaration::AddElement(u32 stream, u32 offset,
                                   VertexElementType type, VertexElementSemantic semantic,
                                   u32 index, u32 divisor)
{
    VertexElement elem;
    elem.stream = stream;
    elem.offset = offset;
    elem.type = type;
    elem.semantic = semantic;
    elem.index = index;
    elem.instanceDivisor = divisor;

    m_elements.push_back(elem);

    u32 endOffset = offset + elem.GetSize();
    auto& streamSize = m_streamSizes[stream];
    if (endOffset > streamSize)
    {
        streamSize = endOffset;
    }

    m_isDirty = true;
}

void VertexDeclaration::Clear()
{
    m_elements.clear();
    m_streamSizes.clear();
    m_isDirty = true;
}

u32 VertexDeclaration::GetVertexSize(u32 stream) const
{
    auto it = m_streamSizes.find(stream);
    return (it != m_streamSizes.end()) ? it->second : 0;
}

const std::vector<VertexElement>& VertexDeclaration::GetElements() const
{
    return m_elements;
}

const VertexElement* VertexDeclaration::FindElement(VertexElementSemantic semantic, u32 index) const
{
    for (const auto& elem : m_elements)
    {
        if (elem.semantic == semantic && elem.index == index)
        {
            return &elem;
        }
    }
    return nullptr;
}

// ============================================================================
// VERTEX BUFFER
// ============================================================================

VertexBuffer::VertexBuffer(u32 vSize, u32 vCount, bool dynamic)
    : m_vbo(0), m_vertexSize(vSize), m_vertexCount(vCount), m_isDynamic(dynamic)
{
    if (vSize == 0 || vCount == 0)
    {
        LogWarning("Creating VertexBuffer with zero size or count\n");
        return;
    }

    CHECK_GL_ERROR(glGenBuffers(1, &m_vbo));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    
    GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, m_vertexSize * m_vertexCount, nullptr, usage));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

VertexBuffer::~VertexBuffer()
{
    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}
 

void VertexBuffer::SetData(const void* data)
{
    if (!IsValid() || !data)
    {
        LogWarning("Invalid VertexBuffer or null data in setData\n");
        return;
    }

    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    
    if (m_isDynamic)
    {
        CHECK_GL_ERROR(glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexSize * m_vertexCount, data));
    }
    else
    {
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, m_vertexSize * m_vertexCount, data, GL_STATIC_DRAW));
    }
    
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::SetSubData(u32 offset, u32 size, const void* data)
{
    if (!IsValid() || !data)
    {
        LogWarning("Invalid VertexBuffer or null data in setSubData\n");
        return;
    }

    if (offset + size > m_vertexSize * m_vertexCount)
    {
        LogError("VertexBuffer::setSubData - offset + size exceeds buffer size\n");
        return;
    }

    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    CHECK_GL_ERROR(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::Bind() const
{
    if (IsValid())
    {
        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    }
}



// ============================================================================
// INDEX BUFFER
// ============================================================================

IndexBuffer::IndexBuffer(u32 iCount, bool dynamic, bool use16Bit)
    : m_ibo(0), m_indexCount(iCount), m_is16Bit(use16Bit), m_isDynamic(dynamic)
{
    if (iCount == 0)
    {
        LogWarning("Creating IndexBuffer with zero count\n");
        return;
    }

    CHECK_GL_ERROR(glGenBuffers(1, &m_ibo));
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));
    
    size_t size = m_indexCount * (m_is16Bit ? sizeof(s16) : sizeof(u32));
    GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    
    CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, usage));
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

IndexBuffer::~IndexBuffer()
{
    if (m_ibo)
    {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
}
 

void IndexBuffer::SetData(const void* data)
{
    if (!IsValid() || !data)
    {
        LogWarning("Invalid IndexBuffer or null data in setData\n");
        return;
    }

    size_t size = m_indexCount * (m_is16Bit ? sizeof(uint16_t) : sizeof(u32));
    
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));
    
    if (m_isDynamic)
    {
        CHECK_GL_ERROR(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data));
    }
    else
    {
        CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    }
    
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void IndexBuffer::SetSubData(u32 offset, u32 count, const void* data)
{
    if (!IsValid() || !data)
    {
        LogWarning("Invalid IndexBuffer or null data in setSubData\n");
        return;
    }

    if (offset + count > m_indexCount)
    {
        LogError("IndexBuffer::setSubData -  count %d  exceeds buffer size %d", offset + count ,m_indexCount);
        return;
    }

    size_t indexSize = m_is16Bit ? sizeof(uint16_t) : sizeof(u32);
    size_t byteOffset = offset * indexSize;
    size_t byteSize = count * indexSize;

    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));
    CHECK_GL_ERROR(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, byteOffset, byteSize, data));
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void IndexBuffer::Bind() const
{
    if (IsValid())
    {
        CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));
    }
}

// void IndexBuffer::unbind()
// {
//     CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
// }

u32 IndexBuffer::GetIndexType() const
{
    return m_is16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

// ============================================================================
// VERTEX ARRAY
// ============================================================================

VertexArray::VertexArray()
    : m_vao(0)
    , m_vertexDeclaration(nullptr)
    , m_indexBuffer(nullptr)
    , m_isBuilt(false)
    , m_needsRebuild(false)
{
    CHECK_GL_ERROR(glGenVertexArrays(1, &m_vao));
    m_vertexDeclaration = new VertexDeclaration();
}

VertexArray::~VertexArray()
{
    Release();
}

  
VertexBuffer* VertexArray::AddVertexBuffer(u32 vSize, u32 vCount, bool dynamic)
{
    if (m_isBuilt)
    {
        LogWarning("Adding vertex buffer after VAO is built - marking for rebuild\n");
        m_needsRebuild = true;
    }

    VertexBuffer* vb = new VertexBuffer(vSize, vCount, dynamic);
    m_vertexBuffers.push_back(vb);
    return vb;
}

IndexBuffer* VertexArray::CreateIndexBuffer(u32 iCount, bool dynamic, bool use16Bit)
{
    if (m_indexBuffer)
    {
        delete m_indexBuffer;
        m_needsRebuild = true;
    }

    m_indexBuffer = new IndexBuffer(iCount, dynamic, use16Bit);
    return m_indexBuffer;
}

void VertexArray::Release()
{
    if (!IsValid())
    {
        return;
    }
     for (auto vb : m_vertexBuffers)
    {
        delete vb;
    }
    m_vertexBuffers.clear();

    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    delete m_vertexDeclaration;
    delete m_indexBuffer;
}

void VertexArray::Build()
{
    if (!IsValid())
    {
        LogError("Cannot build invalid VertexArray\n");
        return;
    }

    CHECK_GL_ERROR(glBindVertexArray(m_vao));

    // Get max vertex attributes supported
    GLint maxAttr = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttr);

    // Reset all attributes
    for (GLint i = 0; i < maxAttr; ++i)
    {
        CHECK_GL_ERROR(glDisableVertexAttribArray(i));
        CHECK_GL_ERROR(glVertexAttribDivisor(i, 0));
    }

    // Setup vertex attributes from declaration
    const auto& elements = m_vertexDeclaration->GetElements();
    GLint location = 0;

    for (const auto& elem : elements)
    {
        // Validate stream index
        if (elem.stream >= m_vertexBuffers.size())
        {
            LogWarning("Invalid stream %u in vertex element (only %zu streams available)\n", 
                      elem.stream, m_vertexBuffers.size());
            continue;
        }

        // Check if we've exceeded max attributes
        if (location >= maxAttr)
        {
            LogError("Too many vertex attributes! Maximum is %d\n", maxAttr);
            break;
        }

        const VertexBuffer* vb = m_vertexBuffers[elem.stream];
        if (!vb || !vb->IsValid())
        {
            LogWarning("Invalid vertex buffer at stream %u\n", elem.stream);
            continue;
        }

        const GLsizei stride = static_cast<GLsizei>(m_vertexDeclaration->GetVertexSize(elem.stream));
        const void* offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(elem.offset));

        vb->Bind();
        CHECK_GL_ERROR(glEnableVertexAttribArray(location));

        // Use integer attribute pointer for blend indices
        const bool forceIntegerAttrib = (elem.semantic == VES_BLEND_INDICES);

        if (forceIntegerAttrib)
        {
            CHECK_GL_ERROR(glVertexAttribIPointer(
                location,
                elem.GetComponentCount(),
                elem.GetType(),
                stride,
                offset
            ));
        }
        else
        {
            CHECK_GL_ERROR(glVertexAttribPointer(
                location,
                elem.GetComponentCount(),
                elem.GetType(),
                elem.ShouldNormalize() ? GL_TRUE : GL_FALSE,
                stride,
                offset
            ));
        }

        CHECK_GL_ERROR(glVertexAttribDivisor(location, elem.instanceDivisor));

        ++location;
    }

    // Bind index buffer (binding is part of VAO state)
    if (m_indexBuffer && m_indexBuffer->IsValid())
    {
        m_indexBuffer->Bind();
    }

    // Cleanup
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECK_GL_ERROR(glBindVertexArray(0));

    m_vertexDeclaration->ClearDirty();
    m_isBuilt = true;
    m_needsRebuild = false;
}

void VertexArray::ensureBuilt() const
{
    if (!m_isBuilt || m_needsRebuild || m_vertexDeclaration->IsDirty())
    {
        const_cast<VertexArray*>(this)->Build();
    }
}

void VertexArray::Render(PrimitiveType type, u32 count) const
{
    if (!IsValid() || m_vertexBuffers.empty() || count == 0)
    {
        return;
    }

    ensureBuilt();

    CHECK_GL_ERROR(glBindVertexArray(m_vao));

    static const GLenum glPrimitiveTypes[] = {
        GL_POINTS,         // PT_POINTS
        GL_LINES,          // PT_LINES
        GL_LINE_STRIP,     // PT_LINE_STRIP
        GL_LINE_LOOP,      // PT_LINE_LOOP
        GL_TRIANGLES,      // PT_TRIANGLES
        GL_TRIANGLE_STRIP, // PT_TRIANGLE_STRIP
        GL_TRIANGLE_FAN    // PT_TRIANGLE_FAN
    };

    const GLenum glMode = glPrimitiveTypes[type];

    if (!ValidateVertexCount(type, count))
    {
        LogWarning("Invalid vertex count %u for primitive type %d\n", count, type);
    }

    if (m_indexBuffer && m_indexBuffer->IsValid())
    {
        const GLenum idxType = m_indexBuffer->GetIndexType();
        Driver::Instance().DrawElements(glMode, count, idxType, nullptr);
        //CHECK_GL_ERROR(glDrawElements(glMode, count, idxType, nullptr));
    }
    else
    {
        Driver::Instance().DrawArrays(glMode, 0, count);
        //CHECK_GL_ERROR(glDrawArrays(glMode, 0, count));
    }

    CHECK_GL_ERROR(glBindVertexArray(0));
}

void VertexArray::RenderInstanced(PrimitiveType type, u32 count, u32 instanceCount) const
{
    if (!IsValid() || m_vertexBuffers.empty() || count == 0 || instanceCount == 0)
    {
        return;
    }

    ensureBuilt();

    CHECK_GL_ERROR(glBindVertexArray(m_vao));

    static const GLenum glPrimitiveTypes[] = {
        GL_POINTS,         // PT_POINTS
        GL_LINES,          // PT_LINES
        GL_LINE_STRIP,     // PT_LINE_STRIP
        GL_LINE_LOOP,      // PT_LINE_LOOP
        GL_TRIANGLES,      // PT_TRIANGLES
        GL_TRIANGLE_STRIP, // PT_TRIANGLE_STRIP
        GL_TRIANGLE_FAN    // PT_TRIANGLE_FAN
    };

    const GLenum glMode = glPrimitiveTypes[type];

    if (!ValidateVertexCount(type, count))
    {
        LogWarning("Invalid vertex count %u for primitive type %d\n", count, type);
    }

    if (m_indexBuffer && m_indexBuffer->IsValid())
    {
        const GLenum idxType = m_indexBuffer->GetIndexType();
        CHECK_GL_ERROR(glDrawElementsInstanced(glMode, count, idxType, nullptr, instanceCount));
    }
    else
    {
        CHECK_GL_ERROR(glDrawArraysInstanced(glMode, 0, count, instanceCount));
    }

    CHECK_GL_ERROR(glBindVertexArray(0));
}