#include "pch.h"
#include "Driver.hpp"
#include "glad/glad.h"
#include "Texture.hpp"
#include "Shader.hpp"



extern u32 CalculatePrimitiveCount(PrimitiveType type, u32 vertexCount);
extern u32 ToGLTextureType(TextureType type);

static GLenum ToGLBlendFactor(BlendFactor f)
{
    static const GLenum table[] = {
        GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
        GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR,
        GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE};
    return table[static_cast<u8>(f)];
}

static GLenum ToGLBlendOp(BlendOp op)
{
    static const GLenum table[] = {
        GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX};
    return table[static_cast<u8>(op)];
}

static GLenum ToGLCompareFunc(CompareFunc f)
{
    static const GLenum table[] = {
        GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS};
    return table[static_cast<u8>(f)];
}

static GLenum ToGLCullMode(CullMode m)
{
    static const GLenum table[] = {
        GL_NONE, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK};
    return table[static_cast<u8>(m)];
}

static GLenum ToGLFrontFace(FrontFace f)
{
    return f == FrontFace::CW ? GL_CW : GL_CCW;
}

static GLenum ToGLStencil(StencilOp op)
{
    switch (op)
    {
    case StencilOp::Keep:
        return GL_KEEP;
    case StencilOp::Zero:
        return GL_ZERO;
    case StencilOp::Replace:
        return GL_REPLACE;
    case StencilOp::Incr:
        return GL_INCR;
    case StencilOp::IncrWrap:
        return GL_INCR_WRAP;
    case StencilOp::Decr:
        return GL_DECR;
    case StencilOp::DecrWrap:
        return GL_DECR_WRAP;
    case StencilOp::Invert:
        return GL_INVERT;
    }
    return GL_KEEP;
}

Driver &Driver::Instance()
{
    static Driver instance;
    return instance;
}

void Driver::DrawMeshBuffer( MeshBuffer *meshBuffer)
{
    m_countMeshBuffer++;
    m_countVertex += meshBuffer->GetVertexCount();
    m_countTriangle += meshBuffer->GetIndexCount() / 3;

    meshBuffer->Render();
}

void Driver::DrawMesh(Mesh *mesh)
{
    m_countMesh++;

    const u32 count = mesh->GetBufferCount();
    for (u32 i = 0; i < count; i++)
    {
        const int materialID = mesh->GetBuffer(i)->GetMaterial();
        if (materialID >= 0 && materialID < (int)mesh->GetMaterialCount())
        {
            const u8 layer = mesh->GetMaterial(materialID)->GetLayers();
            for (u8 i = 0; i < layer; i++)
            {
                const Texture *texture = mesh->GetMaterial(materialID)->GetTexture(i);
                if (texture)
                {
                    texture->Bind(i);
                }
            }
        }
        DrawMeshBuffer(mesh->GetBuffer(i));
    }
 
}

void Driver::DrawElements(u32 mode, u32 count, u32 type, const void *indices)
{
    m_countDrawCall++;
    CHECK_GL_ERROR(glDrawElements(mode, count, type, indices));
}

void Driver::DrawArrays(u32 mode, u32 first, u32 count)
{
    m_countDrawCall++;
    CHECK_GL_ERROR(glDrawArrays(mode, first, count));
}

Driver::Driver()
{
    Reset();
    ShaderManager::Instance().Init();
    TextureManager::Instance().Init();
}

 

void Driver::Reset()
{

    m_countMeshBuffer = 0;
    m_countMesh = 0;
    m_countTriangle = 0;
    m_countVertex = 0;
    m_countDrawCall = 0;
    m_countPrograms = 0;
    m_countTextures = 0;
    m_blendEnabled = false;
    m_blendSrcRGB = m_blendSrcAlpha = BlendFactor::One;
    m_blendDstRGB = m_blendDstAlpha = BlendFactor::Zero;
    m_blendOpRGB = m_blendOpAlpha = BlendOp::Add;

    m_depthTestEnabled = true;
    m_depthWriteEnabled = true;
    m_depthFunc = CompareFunc::Less;

    m_stencilTestEnabled = false;
    m_cullMode = CullMode::Back;
    m_frontFace = FrontFace::CCW;
    m_colorWriteMask = ColorWriteMask::RGBA;

    m_scissorEnabled = false;
    m_polygonOffsetFillEnabled = false;

    m_currentProgram = 0;
    m_currentVAO = 0;
    m_activeTextureUnit = 0;

    for (auto &unit : m_textureUnits)
    {
        unit.tex2D = 0;
        unit.texCube = 0;
        unit.tex2DArray = 0;
        unit.sampler = 0;
    }

    GLint hwUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &hwUnits);
    const int units = (int)std::min<size_t>((size_t)hwUnits, MAX_TEXTURE_UNITS);
    for (int i = 0; i < units; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindSampler(i, 0);
        m_textureUnits[i] = {}; // zera cache
    }
    m_activeTextureUnit = 0;
    glActiveTexture(GL_TEXTURE0);
    m_drawFBO = 0;
    m_readFBO = 0;
    m_arrayBuffer = 0;
    m_elementBuffer = 0;
    m_uniformBuffer = 0;

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void Driver::InvalidateCache()
{
    // Query do OpenGL para sincronizar cache
    GLboolean blend, depthTest, cullFace, scissor;
    GLint prog, vao;

    glGetBooleanv(GL_BLEND, &blend);
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_CULL_FACE, &cullFace);
    glGetBooleanv(GL_SCISSOR_TEST, &scissor);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);

    m_blendEnabled = blend;
    m_depthTestEnabled = depthTest;
    m_scissorEnabled = scissor;
    m_currentProgram = prog;
    m_currentVAO = vao;
}

// ========== BLEND ==========
void Driver::SetBlendEnable(bool enable)
{
    if (CheckAndUpdate(m_blendEnabled, enable))
    {
        enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }
}

void Driver::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    SetBlendFuncSeparate(src, dst, src, dst);
}

void Driver::SetBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB,
                                       BlendFactor srcAlpha, BlendFactor dstAlpha)
{
    bool changed = false;
    changed |= CheckAndUpdate(m_blendSrcRGB, srcRGB);
    changed |= CheckAndUpdate(m_blendDstRGB, dstRGB);
    changed |= CheckAndUpdate(m_blendSrcAlpha, srcAlpha);
    changed |= CheckAndUpdate(m_blendDstAlpha, dstAlpha);

    if (changed)
    {
        glBlendFuncSeparate(
            ToGLBlendFactor(srcRGB), ToGLBlendFactor(dstRGB),
            ToGLBlendFactor(srcAlpha), ToGLBlendFactor(dstAlpha));
    }
}

void Driver::SetBlendOp(BlendOp op)
{
    SetBlendOpSeparate(op, op);
}

void Driver::SetBlendOpSeparate(BlendOp opRGB, BlendOp opAlpha)
{
    bool changed = false;
    changed |= CheckAndUpdate(m_blendOpRGB, opRGB);
    changed |= CheckAndUpdate(m_blendOpAlpha, opAlpha);

    if (changed)
    {
        glBlendEquationSeparate(ToGLBlendOp(opRGB), ToGLBlendOp(opAlpha));
    }
}

// ========== DEPTH ==========
void Driver::SetDepthTest(bool enable)
{
    if (CheckAndUpdate(m_depthTestEnabled, enable))
    {
        enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    }
}

void Driver::SetDepthWrite(bool enable)
{
    if (CheckAndUpdate(m_depthWriteEnabled, enable))
    {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
    }
}

void Driver::SetDepthFunc(CompareFunc f)
{
    if (CheckAndUpdate(m_depthFunc, f))
    {
        glDepthFunc(ToGLCompareFunc(f));
    }
}

// ========== STENCIL ==========
void Driver::SetStencilTest(bool enable)
{
    if (CheckAndUpdate(m_stencilTestEnabled, enable))
    {
        enable ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    }
}

void Driver::SetStencilFunc(CompareFunc func, int ref, u32 mask)
{
    bool changed = false;
    changed |= CheckAndUpdate(m_stencilFunc, func);
    changed |= CheckAndUpdate(m_stencilRef, ref);
    changed |= CheckAndUpdate(m_stencilFuncMask, mask);

    if (changed)
    {
        glStencilFunc(ToGLCompareFunc(func), ref, mask);
    }
}

void Driver::SetStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass)
{
    bool changed = false;
    changed |= CheckAndUpdate(m_stencilOpFail, (u32)fail);
    changed |= CheckAndUpdate(m_stencilOpZFail, (u32)zfail);
    changed |= CheckAndUpdate(m_stencilOpZPass, (u32)zpass);
    if (changed)
        glStencilOp(ToGLStencil(fail), ToGLStencil(zfail), ToGLStencil(zpass));
}

void Driver::SetStencilMask(u32 mask)
{
    if (CheckAndUpdate(m_stencilWriteMask, mask))
    {
        glStencilMask(mask);
    }
}

// ========== CULLING ==========
void Driver::SetCulling(CullMode mode)
{
    if (CheckAndUpdate(m_cullMode, mode))
    {
        if (mode == CullMode::None)
        {
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(ToGLCullMode(mode));
        }
    }
}

void Driver::SetFrontFace(FrontFace ff)
{
    if (CheckAndUpdate(m_frontFace, ff))
    {
        glFrontFace(ToGLFrontFace(ff));
    }
}

// ========== SCISSOR / VIEWPORT ==========
void Driver::SetScissor(bool enable)
{
    if (CheckAndUpdate(m_scissorEnabled, enable))
    {
        enable ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    }
}

void Driver::SetScissorBox(int x, int y, int w, int h)
{
    IntRect newBox = {x, y, w, h};
    if (m_scissorBox != newBox)
    {
        m_scissorBox = newBox;
        glScissor(x, y, w, h);
    }
}

void Driver::SetViewPort(int x, int y, int w, int h)
{
    IntRect newVP = {x, y, w, h};
    if (m_viewport != newVP)
    {
        m_viewport = newVP;
        glViewport(x, y, w, h);
    }
}

// ========== COLOR MASK ==========
void Driver::SetColorWrite(ColorWriteMask m)
{
    if (CheckAndUpdate(m_colorWriteMask, m))
    {
        u8 mask = static_cast<u8>(m);
        glColorMask(
            (mask & static_cast<u8>(ColorWriteMask::R)) != 0,
            (mask & static_cast<u8>(ColorWriteMask::G)) != 0,
            (mask & static_cast<u8>(ColorWriteMask::B)) != 0,
            (mask & static_cast<u8>(ColorWriteMask::A)) != 0);
    }
}

// ========== POLYGON OFFSET ==========
void Driver::SetPolygonOffset(bool enableFill, float factor, float units)
{
    bool changed = false;
    changed |= CheckAndUpdate(m_polygonOffsetFillEnabled, enableFill);
    changed |= CheckAndUpdate(m_polygonOffsetFactor, factor);
    changed |= CheckAndUpdate(m_polygonOffsetUnits, units);

    if (changed)
    {
        if (enableFill)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(factor, units);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }
}

// ========== PROGRAM / VAO ==========
void Driver::UseProgram(u32 program)
{
    if (CheckAndUpdate(m_currentProgram, program))
    {
        glUseProgram(program);
        m_countPrograms++;
    }
}

void Driver::BindVAO(u32 vao)
{
    if (CheckAndUpdate(m_currentVAO, vao))
    {
        glBindVertexArray(vao);
    }
}

// ========== TEXTURES ==========
void Driver::ActiveTextureUnit(u32 unit)
{
    if (CheckAndUpdate(m_activeTextureUnit, unit))
    {
        glActiveTexture(GL_TEXTURE0 + unit);
    }
}

void Driver::BindTexture(u32 unit, u32 target, u32 tex)
{
    if (unit >= MAX_TEXTURE_UNITS)
        return;

    ActiveTextureUnit(unit);

    u32 *cachedTex = nullptr;
    switch (target)
    {
    case GL_TEXTURE_2D:
        cachedTex = &m_textureUnits[unit].tex2D;
        break;
    case GL_TEXTURE_CUBE_MAP:
        cachedTex = &m_textureUnits[unit].texCube;
        break;
    case GL_TEXTURE_2D_ARRAY:
        cachedTex = &m_textureUnits[unit].tex2DArray;
        break;
    default:
 
        glBindTexture(target, tex);
        LogInfo("[TEXTURE] [ID %i] Bound to unit %i", tex, unit);

        return;
    }

    if (CheckAndUpdate(*cachedTex, tex))
    {
        glBindTexture(target, tex);
        m_countTextures++;
    }
}



// ========== FRAMEBUFFERS ==========
void Driver::BindFramebuffer(u32 target, u32 fbo)
{
    bool changed = false;

    switch (target)
    {
    case GL_FRAMEBUFFER:
        changed |= CheckAndUpdate(m_drawFBO, fbo);
        changed |= CheckAndUpdate(m_readFBO, fbo);
        break;
    case GL_DRAW_FRAMEBUFFER:
        changed = CheckAndUpdate(m_drawFBO, fbo);
        break;
    case GL_READ_FRAMEBUFFER:
        changed = CheckAndUpdate(m_readFBO, fbo);
        break;
    }

    if (changed)
    {
        glBindFramebuffer(target, fbo);
    }
}

// ========== BUFFERS ==========
void Driver::BindBuffer(u32 target, u32 buffer)
{
    u32 *cachedBuffer = nullptr;

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        cachedBuffer = &m_arrayBuffer;
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        cachedBuffer = &m_elementBuffer;
        break;
    case GL_UNIFORM_BUFFER:
        cachedBuffer = &m_uniformBuffer;
        break;
    default:
        // Target desconhecido, faz bind direto
        glBindBuffer(target, buffer);

        return;
    }

    if (CheckAndUpdate(*cachedBuffer, buffer))
    {
        glBindBuffer(target, buffer);
    }
}

void Driver::SetClearColor(float r, float g, float b, float a)
{
    if (m_clearColor[0] != r || m_clearColor[1] != g || m_clearColor[2] != b || m_clearColor[3] != a)
    {
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
        m_clearColor[3] = a;
        glClearColor(r, g, b, a);
    }
}

void Driver::SetClearDepth(float d)
{
    d = std::fmax(0.0f, std::fmin(1.0f, d));
    if (m_clearDepth != d)
    {
        m_clearDepth = d;
#if defined(GL_ES_VERSION_2_0) || defined(GL_ES_VERSION_3_0)
        glClearDepthf(d);
#else
        glClearDepth(d);
#endif
    }
}

void Driver::SetClearStencil(int s)
{
    if (m_clearStencil != s)
    {
        m_clearStencil = s;
        glClearStencil(s);
    }
}

void Driver::Clear(unsigned mask)
{
    GLbitfield bits = 0;
    if (mask & CLEAR_COLOR)
        bits |= GL_COLOR_BUFFER_BIT;
    if (mask & CLEAR_DEPTH)
        bits |= GL_DEPTH_BUFFER_BIT;
    if (mask & CLEAR_STENCIL)
        bits |= GL_STENCIL_BUFFER_BIT;
    if (bits == 0)
        return;

    const auto prevColorWrite = m_colorWriteMask;
    const auto prevDepthWrite = m_depthWriteEnabled;

    if (bits & GL_COLOR_BUFFER_BIT)
        SetColorWrite(ColorWriteMask::RGBA);
    if (bits & GL_DEPTH_BUFFER_BIT)
        SetDepthWrite(true);

    glClear(bits);

    if (bits & GL_DEPTH_BUFFER_BIT)
        SetDepthWrite(prevDepthWrite);
    if (bits & GL_COLOR_BUFFER_BIT)
        SetColorWrite(prevColorWrite);
}
