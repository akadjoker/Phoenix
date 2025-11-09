#pragma once
#include "Config.hpp"
#include "Math.hpp"
#include <array>


class Mesh;
class MeshBuffer;

enum class BlendFactor : u8
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};

enum class BlendOp : u8
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

enum class CompareFunc : u8
{
    Never,
    Less,
    Equal,
    LEqual,
    Greater,
    NotEqual,
    GEqual,
    Always
};

enum class CullMode : u8
{
    None,
    Front,
    Back,
    FrontAndBack
};

enum class FrontFace : u8
{
    CW,
    CCW
};

enum class ColorWriteMask : u8
{
    None = 0,
    R = 1,
    G = 2,
    B = 4,
    A = 8,
    RGBA = 15
};

enum class StencilOp : u8
{
    Keep,
    Zero,
    Replace,
    Incr,
    IncrWrap,
    Decr,
    DecrWrap,
    Invert
};

enum class TransformState : int
{
    WORLD = 0,
    VIEW,
    PROJECTION,
    ORTHO,

    VIEW_PROJECTION,          // P*V
    WORLD_VIEW,               // V*M
    WORLD_VIEW_PROJECTION,    // P*V*M

    WORLD_INVERSE,            // inv(M)
    WORLD_INVERSE_TRANSPOSE,  // transpose(inv(M))

    COUNT
};

const unsigned CLEAR_NONE = 0u;
const unsigned CLEAR_COLOR = 1u << 0;
const unsigned CLEAR_DEPTH = 1u << 1;
const unsigned CLEAR_STENCIL = 1u << 2;
const unsigned CLEAR_ALL = (CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL);

class Driver final
{
public:
    static Driver &Instance();

    // ---- Blend ----
    void SetBlendEnable(bool enable);
    void SetBlendFunc(BlendFactor src, BlendFactor dst);
    void SetBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB,
                              BlendFactor srcAlpha, BlendFactor dstAlpha);
    void SetBlendOp(BlendOp op);
    void SetBlendOpSeparate(BlendOp opRGB, BlendOp opAlpha);

    // ---- Depth ----
    void SetDepthTest(bool enable);
    void SetDepthWrite(bool enable);
    void SetDepthFunc(CompareFunc f);

    // ---- Stencil ----
    void SetStencilTest(bool enable);
    void SetStencilFunc(CompareFunc func, int ref, u32 mask);
    void SetStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass);
    void SetStencilMask(u32 mask);

    // ---- Culling / FrontFace ----
    void SetCulling(CullMode mode);
    void SetFrontFace(FrontFace ff);

    // ---- Scissor ----
    void SetScissor(bool enable);
    void SetScissorBox(int x, int y, int w, int h);
    void SetViewPort(int x, int y, int w, int h);

    // ---- Color mask ----
    void SetColorWrite(ColorWriteMask m);

    // ---- Polygon Offset ----
    void SetPolygonOffset(bool enableFill, float factor, float units);

    // ---- Program / VAO ----
    void UseProgram(u32 program);
    void BindVAO(u32 vao);

    // ---- Texturas / Samplers ----
    void ActiveTextureUnit(u32 unit);
    void BindTexture(u32 unit, u32 target, u32 tex);
    void BindSampler(u32 unit, u32 sampler);

    // ---- Framebuffers ----
    void BindFramebuffer(u32 target, u32 fbo);

    // ---- Buffers ----
    void BindBuffer(u32 target, u32 buffer);

    // ---- Clear / Reset ----
    void Reset();
    void InvalidateCache(); // Force resync com OpenGL

    void SetClearColor(float r, float g, float b, float a);
    void SetClearColor(const float rgba[4]) { SetClearColor(rgba[0], rgba[1], rgba[2], rgba[3]); }
    void SetClearDepth(float d);
    void SetClearStencil(int s);

    void Clear(unsigned mask);

    // ---- Queries ----
    bool IsBlendEnabled() const { return m_blendEnabled; }
    bool IsDepthTestEnabled() const { return m_depthTestEnabled; }
    u32 GetCurrentProgram() const { return m_currentProgram; }
    u32 GetCurrentVAO() const { return m_currentVAO; }

    void DrawMeshBuffer( MeshBuffer *meshBuffer);
    void DrawMesh(Mesh *mesh);
 

    void DrawElements(u32 mode, u32 count, u32 type, const void *indices);
    void DrawArrays(u32 mode, u32 first, u32 count);


    u32 GetCountMeshBuffer() const { return m_countMeshBuffer; }
    u32 GetCountMesh() const { return m_countMesh; }
    u32 GetCountTriangle() const { return m_countTriangle; }
    u32 GetCountVertex() const { return m_countVertex; }
    u32 GetCountDrawCall() const { return m_countDrawCall; }
    u32 GetCountPrograms() const { return m_countPrograms; }
    u32 GetCountTextures() const { return m_countTextures; }

    IntRect GetViewport() const { return m_viewport; }
    IntRect GetScissorBox() const { return m_scissorBox; }

    void SaveViewPort();
    void RestoreViewPort();
 


private:
    Driver();
    ~Driver() = default;
    Driver(const Driver &) = delete;
    Driver &operator=(const Driver &) = delete;

    // Helper para aplicar estados OpenGL apenas se mudaram
    template <typename T>
    bool CheckAndUpdate(T &cached, const T &newValue)
    {
        if (cached != newValue)
        {
            cached = newValue;
            return true;
        }
        return false;
    }
  

    float m_clearColor[4] = {0, 0, 0, 0};
    float m_clearDepth = 1.0f;
    int m_clearStencil = 0;
    u32 m_countMeshBuffer = 0;
    u32 m_countMesh = 0;
    u32 m_countTriangle = 0;
    u32 m_countVertex = 0;
    u32 m_countDrawCall = 0;
    u32 m_countPrograms = 0;
    u32 m_countTextures = 0;


    // ---- Blend State ----
    bool m_blendEnabled = false;
    BlendFactor m_blendSrcRGB = BlendFactor::One;
    BlendFactor m_blendDstRGB = BlendFactor::Zero;
    BlendFactor m_blendSrcAlpha = BlendFactor::One;
    BlendFactor m_blendDstAlpha = BlendFactor::Zero;
    BlendOp m_blendOpRGB = BlendOp::Add;
    BlendOp m_blendOpAlpha = BlendOp::Add;

    // ---- Depth State ----
    bool m_depthTestEnabled = true;
    bool m_depthWriteEnabled = true;
    CompareFunc m_depthFunc = CompareFunc::Less;

    // ---- Stencil State ----
    bool m_stencilTestEnabled = false;
    CompareFunc m_stencilFunc = CompareFunc::Always;
    int m_stencilRef = 0;
    u32 m_stencilFuncMask = 0xFFFFFFFF;
    u32 m_stencilOpFail = 0x1E00; // GL_KEEP
    u32 m_stencilOpZFail = 0x1E00;
    u32 m_stencilOpZPass = 0x1E00;
    u32 m_stencilWriteMask = 0xFFFFFFFF;

    // ---- Rasterizer State ----
    CullMode m_cullMode = CullMode::Back;
    FrontFace m_frontFace = FrontFace::CCW;
    bool m_polygonOffsetFillEnabled = false;
    float m_polygonOffsetFactor = 0.0f;
    float m_polygonOffsetUnits = 0.0f;

    // ---- Scissor / Viewport ----
    bool m_scissorEnabled = false;
    IntRect m_scissorBox = {0, 0, 0, 0};
    IntRect m_viewport = {0, 0, 0, 0};
    IntRect m_saveviewport = {0, 0, 0, 0};

    // ---- Color Write ----
    ColorWriteMask m_colorWriteMask = ColorWriteMask::RGBA;

    // ---- Program / VAO ----
    u32 m_currentProgram = 0;
    u32 m_currentVAO = 0;

    // ---- Texturas (cache por unit) ----
    static constexpr size_t MAX_TEXTURE_UNITS = 32;
    u32 m_activeTextureUnit = 0;

    struct TextureBinding
    {
        u32 tex2D = 0;
        u32 texCube = 0;
        u32 tex2DArray = 0;
        u32 sampler = 0;
    };
    std::array<TextureBinding, MAX_TEXTURE_UNITS> m_textureUnits;

    // ---- Framebuffer ----
    u32 m_drawFBO = 0;
    u32 m_readFBO = 0;

    // ---- Buffers ----
    u32 m_arrayBuffer = 0;
    u32 m_elementBuffer = 0;
    u32 m_uniformBuffer = 0;
 
  


};
