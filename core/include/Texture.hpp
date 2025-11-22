#pragma once
#include "Core.hpp"
#include "GraphicsTypes.hpp" 
#include <unordered_map>
#include <string>


class Texture
{
public:
    Texture();
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

    // criação
    bool Create(u32 width, u32 height, TextureFormat format, const void *data = nullptr);
    bool Create3D(u32 width, u32 height, u32 depth, TextureFormat format, const void *data = nullptr);
    bool CreateCube(u32 size, TextureFormat format, const void *faces[6]);

    // I/O
    bool LoadFromFile(const char *path);
    bool LoadCubeFromFiles(const std::vector<std::string> &paths);

    // upload genérico (2D mantém-se; 3D/Array/Cube têm overloads)
    void SetData(const void *data, u32 level = 0);
    void SetSubData(u32 x, u32 y, u32 width, u32 height, const void *data, u32 level = 0);

    // 3D / 2D array
    void SetData3D(u32 level, const void *data);
    void SetSubData3D(u32 x, u32 y, u32 z, u32 w, u32 h, u32 d, const void *data, u32 level = 0);

    // Cubemap (face é GL_TEXTURE_CUBE_MAP_POSITIVE_X + i)
    void SetDataCubeFace(u32 faceIndex, const void *data, u32 level = 0);
    void SetSubDataCubeFace(u32 faceIndex, u32 x, u32 y, u32 w, u32 h, const void *data, u32 level = 0);

    // uso
    void Bind(u32 slot = 0) const;
    void GenerateMipmaps();
    void Release();

    // parâmetros
    void SetMinFilter(FilterMode filter);
    void SetMagFilter(FilterMode filter);
    void SetWrapU(WrapMode wrap);
    void SetWrapV(WrapMode wrap);
    void SetWrapW(WrapMode wrap);
    void SetWrap(WrapMode wrap);
    void SetBorderColor(float r, float g, float b, float a);
    void SetAnisotropy(float value);
    void SetGenerateMipmaps(bool generate);

    // getters
    bool IsValid() const { return m_isValid; }
    u32 GetHandle() const;
    TextureType GetType() const { return m_type; }
    TextureFormat GetFormat() const { return m_format; }
    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    u32 GetDepth() const { return m_depth; }

    FilterMode GetMinFilter() const { return m_minFilter; }
    FilterMode GetMagFilter() const { return m_magFilter; }
    WrapMode GetWrapU() const { return m_wrapU; }
    WrapMode GetWrapV() const { return m_wrapV; }
    WrapMode GetWrapW() const { return m_wrapW; }

    void SetName(const std::string &name) { m_name = name; }
    const std::string &GetName() const { return m_name; }

private:
    friend u32 ToGLTextureType(TextureType);
    friend u32 ToGLFormat(TextureFormat);
    friend u32 GetDataFormat(TextureFormat);
    friend u32 GetDataType(TextureFormat);

    void ApplyParameters();

    u32 m_handle = 0;
    TextureType m_type = TextureType::TEXTURE_2D;
    TextureFormat m_format = TextureFormat::RGBA8;
    u32 m_width = 0, m_height = 0, m_depth = 1;
    u32 m_mipLevels = 1;
    bool m_isValid = false;
    bool m_hasMipmaps = false;
    std::string m_name = "";

    FilterMode m_minFilter = FilterMode::LINEAR;
    FilterMode m_magFilter = FilterMode::LINEAR;
    WrapMode m_wrapU = WrapMode::REPEAT;
    WrapMode m_wrapV = WrapMode::REPEAT;
    WrapMode m_wrapW = WrapMode::REPEAT;
    float m_borderColor[4] = {0, 0, 0, 0};
    float m_maxAnisotropy = 1.0f;
    bool m_generateMipmaps = false;
};

class TextureManager
{
public:
    static TextureManager &Instance();

 
    Texture *Load(const std::string &path, bool generateMipmaps = false);
    Texture *Create(const std::string &name, u32 width, u32 height,TextureFormat format, const void *data = nullptr);

    Texture *Add(const std::string &path,bool generateMipmaps = true);
    Texture *AddCube(const std::string &name,const std::string files[6], bool generateMipmaps = false);

  
    Texture *Get(const std::string &name);

    void SetFlipVerticalOnLoad(bool flip);

 
    void Unload(const std::string &name);
    void UnloadAll();
    bool Exists(const std::string &name) const;


    void SetLoadPath(const std::string &path) { defaultPath = path; }
 
    size_t GetCount() const { return m_textures.size(); }
    void PrintStats() const;

    Texture* GetDefault() { return defaultTexture; }

    void Init();

private:
    TextureManager()=default;
    ~TextureManager() { UnloadAll(); }

    Texture *defaultTexture = nullptr;
    std::string defaultPath;

    std::unordered_map<std::string, Texture *> m_textures;
};