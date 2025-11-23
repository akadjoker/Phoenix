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

enum PatchSize
{
    PATCH_9 = 9,
    PATCH_17 = 17,
    PATCH_33 = 33,
    PATCH_65 = 65,
    PATCH_129 = 129
};
 
struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct VertexSkin
{
    u8 boneIDs[4];
    float weights[4];
};


constexpr u32 MESH_MAGIC = 0x4D455348; // "MESH"
constexpr u32 MESH_VERSION = 100;      // 1.00

constexpr u32 BUFFER_FLAG_SKINNED = 1 << 0;  // Tem skinning data
constexpr u32 BUFFER_FLAG_TANGENTS = 1 << 1; // Tem tangents
constexpr u32 BUFFER_FLAG_COLORS = 1 << 2;   // Tem vertex colors

constexpr u32 CHUNK_MATS = 0x4D415453; // "MATS" - Materials
constexpr u32 CHUNK_BUFF = 0x42554646; // "BUFF" - Buffer
constexpr u32 CHUNK_VRTS = 0x56525453; // "VRTS" - Vertices
constexpr u32 CHUNK_IDXS = 0x49445853; // "IDXS" - Indices
constexpr u32 CHUNK_SKEL = 0x534B454C; // "SKEL" - Skeleton
constexpr u32 CHUNK_SKIN = 0x534B494E; // "SKIN" - Skinning data
constexpr u32 CHUNK_ANIM = 0x414E494D; // "ANIM" - Reserved

constexpr u32 ANIM_MAGIC = 0x414E494D; // "ANIM"
constexpr u32 ANIM_VERSION = 100;      // v1.00

// Chunk IDs
constexpr u32 ANIM_CHUNK_INFO = 0x494E464F; // "INFO" - Animation info
constexpr u32 ANIM_CHUNK_CHAN = 0x4348414E; // "CHAN" - Channel (per bone)
constexpr u32 ANIM_CHUNK_KEYS = 0x4B455953; // "KEYS" - Keyframes

struct ChunkHeader
{
    u32 id;
    u32 length;
};
