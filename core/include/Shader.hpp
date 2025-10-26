#pragma once
#include "Core.hpp"
#include <string>
#include <unordered_map>
#include <cstdint>

class Shader
{
public:
    Shader();
    ~Shader();

    // Delete copy
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    // Move semantics
    Shader(Shader &&other) = delete;
    Shader &operator=(Shader &&other) = delete;

    bool Create(const char *vertexShader, const char *fragmentShader);

    void Release();

    void Bind() const;

    bool addUniform(const char *name);

    bool IsValid() const { return m_Program != 0; }
    u32 Handle() const { return m_Program; }

    int GetUniformLocation(const char *name) const;
    void SetUniform(const char *name, int v) const;
    void SetUniform(const char *name, float v) const;
    void SetUniform(const char *name, float x, float y) const;
    void SetUniform(const char *name, float x, float y, float z) const;
    void SetUniform(const char *name, float x, float y, float z, float w) const;

    void SetUniformMat3(const char *name, const float *m, bool transpose = false) const;
    void SetUniformMat4(const char *name, const float *m, bool transpose = false) const;

    void SetTexture2D(const char *samplerName, u32 texture, int unit) const;

private:
    u32 m_Program;
    mutable std::unordered_map<std::string, int> m_UniformCache;
};

class ShaderManager
{
public:
    static ShaderManager &Instance();

 
    Shader *Load(const std::string &name, const std::string &vertexPath, const std::string &fragmentPath);
    Shader *Create(const std::string &name, const char *vertexShader, const char *fragmentShader);

 
    Shader *Get(const std::string &name);

    void Init();

 
    void UnloadAll();

private:
    ShaderManager() = default;
    ~ShaderManager() { UnloadAll(); }
    static bool ReadTextFileSDL(const char* path, std::string& out);

    std::unordered_map<std::string, Shader *> m_shaders;
};
