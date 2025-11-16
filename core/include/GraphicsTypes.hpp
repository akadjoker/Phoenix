#pragma once
 
enum VertexElementType
{
    VET_FLOAT1,
    VET_FLOAT2,
    VET_FLOAT3,
    VET_FLOAT4,
    VET_COLOR,  // 4 bytes RGBA
    VET_SHORT2, // 2 shorts
    VET_SHORT4, // 4 shorts
    VET_UBYTE4  // 4 unsigned bytes
};

enum VertexElementSemantic
{
    VES_POSITION = 0,
    VES_TEXCOORD,
    VES_COLOR,
    VES_NORMAL,
    VES_TANGENT,
    VES_BINORMAL,
    VES_BLEND_WEIGHTS,
    VES_BLEND_INDICES
};

enum PrimitiveType
{
    PT_POINTS,
    PT_LINES,
    PT_LINE_STRIP,
    PT_LINE_LOOP,
    PT_TRIANGLES,
    PT_TRIANGLE_STRIP,
    PT_TRIANGLE_FAN
};
// Texture types
enum class TextureType
{
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBE,
    TEXTURE_2D_ARRAY
};

// Texture formats
enum class TextureFormat
{
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    DEPTH16,
    DEPTH24,
    DEPTH32F,
    DEPTH24_STENCIL8
};

// Texture filtering
enum class FilterMode
{
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP,
    LINEAR_MIPMAP
};

// Texture wrapping
enum class WrapMode
{
    REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRROR_REPEAT
};

enum class AttachmentType
{
    COLOR,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL
};

// ============================================
// ENUMS & TYPES
// ============================================

enum class RTError {
    NONE,
    INVALID_SIZE,
    ATTACHMENT_LIMIT,
    UNSUPPORTED_FORMAT,
    INCOMPLETE_FBO,
    CREATION_FAILED,
    NOT_FINALIZED,
    INVALID_ATTACHMENT
};

enum class RenderPixelFormat {
    RGB,
    RGBA,
    RED,
    RG,
    DEPTH,
    DEPTH_STENCIL
};

enum class PixelType {
    UNSIGNED_BYTE,
    FLOAT,
    UNSIGNED_INT_24_8
};

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