#include "pch.h"
#include "Texture.hpp"
#include "stb_image.h"
#include "Driver.hpp"

// --- GL helpers (assinaturas no header como friend)
static inline bool IsDepthFormat(TextureFormat f) { return f >= TextureFormat::DEPTH16; }

u32 ToGLTextureType(TextureType type)
{
    switch (type)
    {
    case TextureType::TEXTURE_2D:
        return GL_TEXTURE_2D;
    case TextureType::TEXTURE_3D:
        return GL_TEXTURE_3D;
    case TextureType::TEXTURE_CUBE:
        return GL_TEXTURE_CUBE_MAP;
    case TextureType::TEXTURE_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;
    default:
        return GL_TEXTURE_2D;
    }
}
u32 ToGLFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8:
        return GL_R8;
    case TextureFormat::RG8:
        return GL_RG8;
    case TextureFormat::RGB8:
        return GL_RGB8;
    case TextureFormat::RGBA8:
        return GL_RGBA8;
    case TextureFormat::R16F:
        return GL_R16F;
    case TextureFormat::RG16F:
        return GL_RG16F;
    case TextureFormat::RGB16F:
        return GL_RGB16F;
    case TextureFormat::RGBA16F:
        return GL_RGBA16F;
    case TextureFormat::R32F:
        return GL_R32F;
    case TextureFormat::RG32F:
        return GL_RG32F;
    case TextureFormat::RGB32F:
        return GL_RGB32F;
    case TextureFormat::RGBA32F:
        return GL_RGBA32F;
    case TextureFormat::DEPTH16:
        return GL_DEPTH_COMPONENT16;
    case TextureFormat::DEPTH24:
        return GL_DEPTH_COMPONENT24;
    case TextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT32F;
    case TextureFormat::DEPTH24_STENCIL8:
        return GL_DEPTH24_STENCIL8;
    default:
        return GL_RGBA8;
    }
}
u32 GetDataFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8:
    case TextureFormat::R16F:
    case TextureFormat::R32F:
        return GL_RED;
    case TextureFormat::RG8:
    case TextureFormat::RG16F:
    case TextureFormat::RG32F:
        return GL_RG;
    case TextureFormat::RGB8:
    case TextureFormat::RGB16F:
    case TextureFormat::RGB32F:
        return GL_RGB;
    case TextureFormat::RGBA8:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
        return GL_RGBA;
    case TextureFormat::DEPTH16:
    case TextureFormat::DEPTH24:
    case TextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT;
    case TextureFormat::DEPTH24_STENCIL8:
        return GL_DEPTH_STENCIL;
    default:
        return GL_RGBA;
    }
}
u32 GetDataType(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8:
    case TextureFormat::RG8:
    case TextureFormat::RGB8:
    case TextureFormat::RGBA8:
        return GL_UNSIGNED_BYTE;
    case TextureFormat::R16F:
    case TextureFormat::RG16F:
    case TextureFormat::RGB16F:
    case TextureFormat::RGBA16F:
        return GL_HALF_FLOAT;
    case TextureFormat::R32F:
    case TextureFormat::RG32F:
    case TextureFormat::RGB32F:
    case TextureFormat::RGBA32F:
    case TextureFormat::DEPTH32F:
        return GL_FLOAT;
    case TextureFormat::DEPTH16:
        return GL_UNSIGNED_SHORT;
    case TextureFormat::DEPTH24:
        return GL_UNSIGNED_INT;
    case TextureFormat::DEPTH24_STENCIL8:
        return GL_UNSIGNED_INT_24_8;
    default:
        return GL_UNSIGNED_BYTE;
    }
}

// ------------------

Texture::Texture() {}
Texture::~Texture() { Release(); }

Texture::Texture(Texture &&o) noexcept { *this = std::move(o); }
Texture &Texture::operator=(Texture &&o) noexcept
{
    if (this != &o)
    {
        Release();
        m_handle = o.m_handle;
        o.m_handle = 0;
        m_type = o.m_type;
        m_format = o.m_format;
        m_width = o.m_width;
        m_height = o.m_height;
        m_depth = o.m_depth;
        m_mipLevels = o.m_mipLevels;
        m_isValid = o.m_isValid;
        m_hasMipmaps = o.m_hasMipmaps;
        m_minFilter = o.m_minFilter;
        m_magFilter = o.m_magFilter;
        m_wrapU = o.m_wrapU;
        m_wrapV = o.m_wrapV;
        m_wrapW = o.m_wrapW;
        memcpy(m_borderColor, o.m_borderColor, sizeof(m_borderColor));
        m_maxAnisotropy = o.m_maxAnisotropy;
        m_generateMipmaps = o.m_generateMipmaps;

        // reset do “o”
        o.m_isValid = false;
        o.m_hasMipmaps = false;
        o.m_width = o.m_height = o.m_depth = 0;
        o.m_mipLevels = 1;
    }
    return *this;
}

void Texture::SetGenerateMipmaps(bool generate)
{
    m_generateMipmaps = generate;
    if (m_isValid && generate && !IsDepthFormat(m_format))
    {
        GenerateMipmaps();
        ApplyParameters();
    }
}

u32 Texture::GetHandle() const
{
    if (!IsValid())
    {
        LogWarning("[Texture] Handle invalid");
        return 0;
    }
    return m_handle;
}

void Texture::SetData(const void *data, u32 level)
{
    if (!IsValid() || !data)
    {
        LogWarning("Texture::SetData invalid");
        return;
    }
    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);

    // alinhamento seguro para qualquer largura
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (m_type == TextureType::TEXTURE_2D)
    {
        glTexSubImage2D(target, level, 0, 0, m_width, m_height,
                        GetDataFormat(m_format), GetDataType(m_format), data);
    }
    else if (m_type == TextureType::TEXTURE_3D || m_type == TextureType::TEXTURE_2D_ARRAY)
    {
        glTexSubImage3D(target, level, 0, 0, 0, m_width, m_height, m_depth,
                        GetDataFormat(m_format), GetDataType(m_format), data);
    }
    else
    {
        LogError("[Texture] Use SetDataCubeFace para cubemaps");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(target, 0);
}

void Texture::SetSubData(u32 x, u32 y, u32 w, u32 h, const void *data, u32 level)
{
    if (!IsValid() || !data)
    {
        LogWarning("Texture::SetSubData invalid");
        return;
    }
    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (m_type == TextureType::TEXTURE_2D)
    {
        glTexSubImage2D(target, level, x, y, w, h,
                        GetDataFormat(m_format), GetDataType(m_format), data);
    }
    else if (m_type == TextureType::TEXTURE_3D || m_type == TextureType::TEXTURE_2D_ARRAY)
    {
        glTexSubImage3D(target, level, x, y, 0, w, h, 1,
                        GetDataFormat(m_format), GetDataType(m_format), data);
    }
    else
    {
        LogError("[Texture] Use SetSubDataCubeFace para cubemaps");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(target, 0);
}

void Texture::SetData3D(u32 level, const void *data)
{
    if (!IsValid() || !data || (m_type != TextureType::TEXTURE_3D && m_type != TextureType::TEXTURE_2D_ARRAY))
    {
        LogWarning("Texture::SetData3D invalid");
        return;
    }
    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(target, level, 0, 0, 0, m_width, m_height, m_depth,
                    GetDataFormat(m_format), GetDataType(m_format), data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(target, 0);
}

void Texture::SetSubData3D(u32 x, u32 y, u32 z, u32 w, u32 h, u32 d, const void *data, u32 level)
{
    if (!IsValid() || !data || (m_type != TextureType::TEXTURE_3D && m_type != TextureType::TEXTURE_2D_ARRAY))
    {
        LogWarning("Texture::SetSubData3D invalid");
        return;
    }
    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(target, level, x, y, z, w, h, d,
                    GetDataFormat(m_format), GetDataType(m_format), data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(target, 0);
}

void Texture::SetDataCubeFace(u32 faceIndex, const void *data, u32 level)
{
    if (!IsValid() || !data || m_type != TextureType::TEXTURE_CUBE || faceIndex > 5)
    {
        LogWarning("Texture::SetDataCubeFace invalid");
        return;
    }
    const GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(face, level, 0, 0, m_width, m_height,
                    GetDataFormat(m_format), GetDataType(m_format), data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
void Texture::SetSubDataCubeFace(u32 faceIndex, u32 x, u32 y, u32 w, u32 h, const void *data, u32 level)
{
    if (!IsValid() || !data || m_type != TextureType::TEXTURE_CUBE || faceIndex > 5)
    {
        LogWarning("Texture::SetSubDataCubeFace invalid");
        return;
    }
    const GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(face, level, x, y, w, h,
                    GetDataFormat(m_format), GetDataType(m_format), data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Texture::Bind(u32 slot) const
{
    
    if (!IsValid())
    {
        LogWarning("[Texture] Handle invalid");
        return ;
    }
    
 //   glActiveTexture(GL_TEXTURE0 + slot);
//    glBindTexture(ToGLTextureType(m_type), m_handle);
   //  LogInfo("[Texture] Binding to slot %u with handle %u", slot, m_handle);

     Driver::Instance().BindTexture(slot, ToGLTextureType(m_type), m_handle);
}

void Texture::Release()
{
    if (m_handle)
    {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
    m_isValid = false;
    m_hasMipmaps = false;
}

void Texture::GenerateMipmaps()
{
    if (!m_isValid || IsDepthFormat(m_format))
        return;
    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glGenerateMipmap(target);
    glBindTexture(target, 0);
    m_hasMipmaps = true;
}

bool Texture::Create(u32 width, u32 height, TextureFormat format, const void *data)
{
    if (width == 0 || height == 0)
    {
        LogError("[Texture] invalid size");
        return false;
    }
    
    if (IsDepthFormat(format) && data)
        LogWarning("[Texture] depth texture with data");
    
    Release();
    
    m_type = TextureType::TEXTURE_2D;
    m_width = width;
    m_height = height;
    m_depth = 1;
    m_format = format;
    m_hasMipmaps = false;
    m_mipLevels = (m_generateMipmaps && data) ? (u32)std::floor(std::log2(std::max(width, height))) + 1 : 1;
    
    glGenTextures(1, &m_handle);
    if (!m_handle)
    {
        LogError("[Texture] glGenTextures failed");
        return false;
    }
    
    const GLenum target = ToGLTextureType(m_type);
    const GLenum internalFmt = ToGLFormat(format);
    
    glBindTexture(target, m_handle);
    
    // Swizzles úteis para R*/RG*
    if (format == TextureFormat::R8 || format == TextureFormat::R16F || format == TextureFormat::R32F)
    {
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
    }
    else if (format == TextureFormat::RG8 || format == TextureFormat::RG16F || format == TextureFormat::RG32F)
    {
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
    }
    
    // SUGESTÃO 1: Verificar erros do glTexStorage2D
    glTexStorage2D(target, m_mipLevels, internalFmt, width, height);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        LogError("[Texture] glTexStorage2D failed: 0x%x", err);
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
        glBindTexture(target, 0);
        return false;
    }
    
    if (data)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(target, 0, 0, 0, width, height, 
                        GetDataFormat(format), GetDataType(format), data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        
        // SUGESTÃO 2: Verificar erro do upload
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            LogError("[Texture] glTexSubImage2D failed: 0x%x", err);
        }
        
        if (m_generateMipmaps && m_mipLevels > 1)
        {
            glGenerateMipmap(target);
            m_hasMipmaps = true;
        }
    }
    
    m_isValid = true;
    ApplyParameters();
    
    glBindTexture(target, 0);
    
    LogInfo("[Texture] Created %u (%ux%u, mips=%u, fmt=%d)", 
            m_handle, width, height, m_mipLevels, (int)format);
    
    return true;
}

bool Texture::Create3D(u32 width, u32 height, u32 depth, TextureFormat format, const void *data)
{
    if (!width || !height || !depth)
    {
        LogError("[Texture] invalid 3D size");
        return false;
    }

    Release();
    m_type = TextureType::TEXTURE_3D;
    m_width = width;
    m_height = height;
    m_depth = depth;
    m_format = format;
    m_hasMipmaps = false;

    u32 maxDim = std::max(width, std::max(height, depth));
    m_mipLevels = (m_generateMipmaps && data) ? (u32)std::floor(std::log2(maxDim)) + 1 : 1;

    glGenTextures(1, &m_handle);
    if (!m_handle)
    {
        LogError("[Texture] glGenTextures 3D failed");
        return false;
    }

    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glTexStorage3D(target, m_mipLevels, ToGLFormat(format), width, height, depth);

    if (data)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage3D(target, 0, 0, 0, 0, width, height, depth,
                        GetDataFormat(format), GetDataType(format), data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        if (m_generateMipmaps && m_mipLevels > 1)
        {
            glGenerateMipmap(target);
            m_hasMipmaps = true;
        }
    }

    m_isValid = true;
    ApplyParameters();
    glBindTexture(target, 0);

    LogInfo("[Texture] Created 3D %u (%ux%ux%u, mips=%u)", m_handle, width, height, depth, m_mipLevels);
    return true;
}

bool Texture::CreateCube(u32 size, TextureFormat format, const void *faces[6])
{
    if (!size)
    {
        LogError("[Texture] invalid cube size");
        return false;
    }

    Release();
    m_type = TextureType::TEXTURE_CUBE;
    m_width = size;
    m_height = size;
    m_depth = 6;
    m_format = format;
    m_hasMipmaps = false;

    const bool hasFaceData = faces && faces[0];
    m_mipLevels = (m_generateMipmaps && hasFaceData) ? (u32)std::floor(std::log2(size)) + 1 : 1;

    glGenTextures(1, &m_handle);
    if (!m_handle)
    {
        LogError("[Texture] glGenTextures cube failed");
        return false;
    }

    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);
    glTexStorage2D(target, m_mipLevels, ToGLFormat(format), size, size);

    if (faces)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        const GLenum cubeTargets[6] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
        for (int i = 0; i < 6; i++)
            if (faces[i])
            {
                glTexSubImage2D(cubeTargets[i], 0, 0, 0, size, size,
                                GetDataFormat(format), GetDataType(format), faces[i]);
            }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        if (m_generateMipmaps && m_mipLevels > 1 && hasFaceData)
        {
            glGenerateMipmap(target);
            m_hasMipmaps = true;
        }
    }

    m_isValid = true;
    ApplyParameters();
    glBindTexture(target, 0);

    LogInfo("[Texture] Created cubemap %u (%u, mips=%u)", m_handle, size, m_mipLevels);
    return true;
}

bool Texture::LoadFromFile(const char *path)
{
    const bool isHDR = stbi_is_hdr(path) != 0;
    int w, h, ch;
    void *img = nullptr;

    stbi_set_flip_vertically_on_load(true);
    if (isHDR)
        img = stbi_loadf(path, &w, &h, &ch, 0);
    else
        img = stbi_load(path, &w, &h, &ch, 0);

    if (!img)
    {
        LogError("[Texture] load failed: %s (%s)", path, stbi_failure_reason());
        return false;
    }

    TextureFormat fmt;
    if (isHDR)
    {
        fmt = (ch == 1) ? TextureFormat::R16F : (ch == 2) ? TextureFormat::RG16F
                                            : (ch == 3)   ? TextureFormat::RGB16F
                                                          : TextureFormat::RGBA16F;
    }
    else
    {
        fmt = (ch == 1) ? TextureFormat::R8 : (ch == 2) ? TextureFormat::RG8
                                          : (ch == 3)   ? TextureFormat::RGB8
                                                        : TextureFormat::RGBA8;
    }

    const bool ok = Create((u32)w, (u32)h, fmt, img);
    stbi_image_free(img);
    return ok;
}

bool Texture::LoadCubeFromFiles(const std::vector<std::string> &paths)
{
    int w[6]{}, h[6]{}, ch[6]{};
    std::vector<stbi_uc *> data;
    data.reserve(6);
    bool ok = true;

    stbi_set_flip_vertically_on_load(true);
    for (int i = 0; i < 6; i++)
    {
        data[i] = stbi_load(paths[i].c_str(), &w[i], &h[i], &ch[i], 0);
        if (!data[i])
        {
            LogError("[Texture] cube face load failed: %s", paths[i]);
            ok = false;
            break;
        }
        if (i > 0 && (w[i] != w[0] || h[i] != h[0]))
        {
            LogError("[Texture] cube faces size mismatch");
            ok = false;
            break;
        }
    }
    if (!ok)
    {
        for (auto *p : data)
            if (p)
                stbi_image_free(p);
        return false;
    }

    TextureFormat fmt;
    switch (ch[0])
    {
    case 1:
        fmt = TextureFormat::R8;
        break;
    case 2:
        fmt = TextureFormat::RG8;
        break;
    case 3:
        fmt = TextureFormat::RGB8;
        break;
    default:
        fmt = TextureFormat::RGBA8;
        break;
    }

    const void *faces[6] = {data[0], data[1], data[2], data[3], data[4], data[5]};
    bool created = CreateCube((u32)w[0], fmt, faces);

    for (auto *p : data)
        if (p)
            stbi_image_free(p);
    return created;
}

void Texture::SetMinFilter(FilterMode f)
{
    m_minFilter = f;
    if (m_isValid)
        ApplyParameters();
}
void Texture::SetMagFilter(FilterMode f)
{
    m_magFilter = f;
    if (m_isValid)
        ApplyParameters();
}

void Texture::SetWrap(WrapMode w)
{
    m_wrapU = m_wrapV = m_wrapW = w;
    if (m_isValid)
        ApplyParameters();
}
void Texture::SetWrapU(WrapMode w)
{
    m_wrapU = w;
    if (m_isValid)
        ApplyParameters();
}
void Texture::SetWrapV(WrapMode w)
{
    m_wrapV = w;
    if (m_isValid)
        ApplyParameters();
}
void Texture::SetWrapW(WrapMode w)
{
    m_wrapW = w;
    if (m_isValid)
        ApplyParameters();
}

void Texture::SetBorderColor(float r, float g, float b, float a)
{
    m_borderColor[0] = r;
    m_borderColor[1] = g;
    m_borderColor[2] = b;
    m_borderColor[3] = a;
    if (m_isValid)
        ApplyParameters();
}

void Texture::SetAnisotropy(float value)
{
    m_maxAnisotropy = value;
    if (m_isValid)
        ApplyParameters();
}

void Texture::ApplyParameters()
{
    if (!m_isValid)
        return;

    const GLenum target = ToGLTextureType(m_type);
    glBindTexture(target, m_handle);

    // min filter coerente com disponibilidade de mips
    GLenum minFilterGL;
    if (m_hasMipmaps && (m_minFilter == FilterMode::NEAREST_MIPMAP || m_minFilter == FilterMode::LINEAR_MIPMAP))
        minFilterGL = (m_minFilter == FilterMode::NEAREST_MIPMAP) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
    else
        minFilterGL = (m_minFilter == FilterMode::NEAREST_MIPMAP) ? GL_NEAREST : (m_minFilter == FilterMode::LINEAR_MIPMAP) ? GL_LINEAR
                                                                             : (m_minFilter == FilterMode::NEAREST)         ? GL_NEAREST
                                                                                                                            : GL_LINEAR;

    GLenum magFilterGL = (m_magFilter == FilterMode::NEAREST) ? GL_NEAREST : GL_LINEAR;

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterGL);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterGL);

    glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLint)(m_wrapU == WrapMode::REPEAT ? GL_REPEAT : m_wrapU == WrapMode::CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE
                                                                                             : m_wrapU == WrapMode::CLAMP_TO_BORDER ? GL_CLAMP_TO_BORDER
                                                                                                                                    : GL_MIRRORED_REPEAT));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLint)(m_wrapV == WrapMode::REPEAT ? GL_REPEAT : m_wrapV == WrapMode::CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE
                                                                                             : m_wrapV == WrapMode::CLAMP_TO_BORDER ? GL_CLAMP_TO_BORDER
                                                                                                                                    : GL_MIRRORED_REPEAT));

    if (m_type == TextureType::TEXTURE_3D || m_type == TextureType::TEXTURE_2D_ARRAY || m_type == TextureType::TEXTURE_CUBE)
    {
        glTexParameteri(target, GL_TEXTURE_WRAP_R, (GLint)(m_wrapW == WrapMode::REPEAT ? GL_REPEAT : m_wrapW == WrapMode::CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE
                                                                                                 : m_wrapW == WrapMode::CLAMP_TO_BORDER ? GL_CLAMP_TO_BORDER
                                                                                                                                        : GL_MIRRORED_REPEAT));
    }

    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, m_borderColor);

    // anisotrópico (se suportado)
    if (m_maxAnisotropy > 1.0f)
    {
        static bool checked = false, hasExt = false;
        if (!checked)
        {
            const char *ext = (const char *)glGetString(GL_EXTENSIONS);
            hasExt = (ext && strstr(ext, "GL_EXT_texture_filter_anisotropic"));
            checked = true;
        }
        if (hasExt)
        {
            float maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            float aniso = std::min(m_maxAnisotropy, maxAniso);
            glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        }
    }

    glBindTexture(target, 0);
}

TextureManager &TextureManager::Instance()
{
    static TextureManager instance;
    return instance;
}

Texture *TextureManager::Load(const std::string &path, bool generateMipmaps)
{
    
    std::string name = Utils::GetFileNameWithoutExt(path.c_str());
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        return it->second;
    }

 
    Texture *texture = new Texture();
    texture->SetGenerateMipmaps(generateMipmaps);
    texture->SetName(Utils::GetFileName(path.c_str()));
    texture->SetAnisotropy(8.0f);

    if (!texture->LoadFromFile(path.c_str()))
    {
        LogError("[TextureManager] Failed to load: %s", path.c_str());
        delete texture;
        return nullptr;
    }
    m_textures[name] = texture;
    LogInfo("[TextureManager] Loaded: %s", name.c_str());
    return texture;
}

Texture *TextureManager::Create(const std::string &name, u32 width, u32 height,
                                       TextureFormat format, const void *data)
{
     
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        LogWarning("[TextureManager] Texture already exists: %s", name.c_str());
        return it->second;
    }

 
    Texture *texture = new Texture();
    texture->SetName(name);
    if (!texture->Create(width, height, format, data))
    {
        LogError("[TextureManager] Failed to create: %s", name.c_str());
        delete texture;
        return nullptr;
    }

    m_textures[name] = texture;
    LogInfo("[TextureManager] Created: %s (%ux%u)", name.c_str(), width, height);
    return texture;
}

Texture *TextureManager::Add(const std::string &path, bool generateMipmaps)
{
    std::string finalePath = defaultPath + path;
    if (Utils::FileExists(finalePath.c_str()))
    {
        LogInfo("[TextureManager] Loading texture: %s",  finalePath.c_str());
        return Load(finalePath, generateMipmaps);
    }
    LogError("[TextureManager] Texture not found: %s", finalePath.c_str());
    return nullptr;
}

Texture *TextureManager::Get(const std::string &name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        return it->second;
    }
    LogWarning("[TextureManager] Texture not found: %s", name.c_str());
    return nullptr;
}

void TextureManager::Unload(const std::string &name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        delete it->second;
        m_textures.erase(it);
        LogInfo("[TextureManager] Unloaded: %s", name.c_str());
    }
}

void TextureManager::UnloadAll()
{
    if (m_textures.empty())
    {
        return;
    }
    for (auto &[name, texture] : m_textures)
    {
        delete texture;
    }
    m_textures.clear();
    LogInfo("[TextureManager] Unloaded all textures");
}

bool TextureManager::Exists(const std::string &name) const
{
    return m_textures.find(name) != m_textures.end();
}

void TextureManager::PrintStats() const
{
    LogInfo("[TextureManager] Loaded textures: %zu", m_textures.size());
    for (const auto &[name, texture] : m_textures)
    {
        LogInfo("  %s (%ux%u)", name.c_str(), texture->GetWidth(), texture->GetHeight());
    }
}

void TextureManager::Init()
{
    defaultPath ="assets/textures/";
    u8 whitePixel[] = {255, 255, 255, 255};
    defaultTexture = Create("white", 1, 1, TextureFormat::RGBA8, whitePixel);

 

    u32 size = 128;
    u32 squareSize = 16;
    std::vector<u8> pixels(size * size * 3); // RGB format

    for (u32 y = 0; y < size; y++)
    {
        for (u32 x = 0; x < size; x++)
        {
         
            u32 squareX = x / squareSize;
            u32 squareY = y / squareSize;

           
            u8 color = ((squareX + squareY) % 2 == 0) ? 255 : 0; // White or Black

            u32 index = (y * size + x) * 3;
            pixels[index + 0] = color; // R
            pixels[index + 1] = color; // G
            pixels[index + 2] = color; // B
        }
    }

    Create("checker", size, size, TextureFormat::RGB8, pixels.data());
}
