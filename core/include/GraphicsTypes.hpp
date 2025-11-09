#pragma once
 

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