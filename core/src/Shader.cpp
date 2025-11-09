#include "pch.h"
#include "Shader.hpp"
#include "Driver.hpp"

u32 CompileShader(GLenum stage, const char *source)
{
    u32 s = glCreateShader(stage);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);

    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log;
        log.resize(len > 1 ? len : 1);
        if (len > 1)
            glGetShaderInfoLog(s, len, nullptr, log.data());
        LogError("[Shader Compile] %s %s", log.c_str(), (stage == GL_VERTEX_SHADER ? "vertex" : "fragment"));
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static bool LinkShader(u32 program)
{
    glLinkProgram(program);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE)
    {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log;
        log.resize(len > 1 ? len : 1);
        if (len > 1)
            glGetProgramInfoLog(program, len, nullptr, log.data());
        LogError("[Program Link] %s", log.c_str());
        return false;
    }
    return true;
}

Shader::Shader()
{
    m_Program = 0;
}

Shader::~Shader()
{
    if (m_Program)
        Release();
}

bool Shader::Create(const char *vertexShader, const char *fragmentShader)
{

    if (!vertexShader || !fragmentShader)
    {
        LogError("[Shader] Null shader source provided");
        return false;
    }

    Release();

    u32 vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    if (!vs)
        return false;
    u32 fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    if (!fs)
    {

        glDeleteShader(vs);
        return false;
    }

    m_Program = glCreateProgram();
    glAttachShader(m_Program, vs);
    glAttachShader(m_Program, fs);

    const bool linked = LinkShader(m_Program);
    if (!linked)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(m_Program);

    LogInfo("[SHADER] [ID %i] Created", m_Program);

    int uniformCount = -1;
    glGetProgramiv(m_Program, GL_ACTIVE_UNIFORMS, &uniformCount);

    for (int i = 0; i < uniformCount; i++)
    {
        int namelen = -1;
        int num = -1;
        char name[256];
        GLenum type = GL_ZERO;
        glGetActiveUniform(m_Program, i, sizeof(name) - 1, &namelen, &num, &type, name);
        name[namelen] = 0;
        addUniform(((char *)&name[0]));
        LogInfo("[SHADER] [ID %i] Active uniform (%s) set at location: %i", m_Program, name, glGetUniformLocation(m_Program, name));
    }

    return linked;
}

void Shader::Release()
{
    if (m_Program)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
        LogInfo("[SHADER] [ID %i] Released", m_Program);
    }
}

bool Shader::addUniform(const char *name)
{
    int location = -1;
    location = glGetUniformLocation(m_Program, name);
    if (location == -1)
    {
        LogError("[SHADER] [ID %i] Failed to find shader uniform: %s", m_Program, name);
        return false;
    }
    m_UniformCache.emplace(name, location);

    //  LogInfo( "SHADER: [ID %i] shader uniform (%s) set at location: %i", m_program, name.c_str(), location);

    return true;
}

void Shader::Bind() const
{
    if (!m_Program)
        return;
    //glUseProgram(m_Program);
    Driver::Instance().UseProgram(m_Program);
}

int Shader::GetUniformLocation(const char *name) const
{
    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end())
        return it->second;
    int loc = glGetUniformLocation(m_Program, name);
    if (loc < 0)
        return loc;
    LogWarning("[SHADER] [ID %i] Failed to find shader uniform: %s", m_Program, name);
    m_UniformCache.emplace(name, loc);
    return loc;
}

void Shader::SetUniform(const char *name, int v) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniform1i(loc, v);
}
void Shader::SetUniform(const char *name, float v) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniform1f(loc, v);
    
}
void Shader::SetUniform(const char *name, float x, float y) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniform2f(loc, x, y);
}
void Shader::SetUniform(const char *name, float x, float y, float z) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniform3f(loc, x, y, z);
}
void Shader::SetUniform(const char *name, float x, float y, float z, float w) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniform4f(loc, x, y, z, w);
}

void Shader::SetUniformMat3(const char *name, const float *m, bool transpose) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniformMatrix3fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, m);
}
void Shader::SetUniformMat4(const char *name, const float *m, bool transpose) const
{
    GLint loc = GetUniformLocation(name);
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, m);
}

void Shader::SetTexture2D(const char *samplerName, u32 texture, int unit) const
{
    GLint loc = GetUniformLocation(samplerName);
    if (loc < 0)
        return;
    glUniform1i(loc, unit);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

bool ShaderManager::ReadTextFileSDL(const char *path, std::string &out)
{
    if (!path || !*path)
        return false;
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    if (!rw)
    {
        LogError("[ShaderManager] Shader file open fail: %s (%s)", path, SDL_GetError());
        return false;
    }
    Sint64 size = SDL_RWsize(rw);
    if (size < 0)
    {
        SDL_RWclose(rw);
        return false;
    }
    out.resize((size_t)size);
    size_t readTotal = 0;
    while (readTotal < (size_t)size)
    {
        size_t toRead = (size_t)size - readTotal;
        size_t got = SDL_RWread(rw, out.data() + readTotal, 1, toRead);
        if (got == 0)
            break;
        readTotal += got;
    }
    SDL_RWclose(rw);
    if (readTotal != (size_t)size)
    {
        out.resize(readTotal);
    }

    if (out.empty() || out.back() != '\0')
        out.push_back('\0');
    return true;
}

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager instance;
    return instance;
}

Shader *ShaderManager::Load(const std::string &name, const std::string &vertexPath, const std::string &fragmentPath)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
        return it->second;
    Shader *shader = new Shader();
    std::string vs, fs;
    if (!ReadTextFileSDL(vertexPath.c_str(), vs) || !ReadTextFileSDL(fragmentPath.c_str(), fs))
    {
        LogError("[ShaderManager] Failed to load shader: %s (%s, %s)", name.c_str(), vertexPath.c_str(), fragmentPath.c_str());
        return 0;
    }
    if (!shader->Create(vs.c_str(), fs.c_str()))
    {
        LogError("[ShaderManager] Failed to create shader: %s (%s, %s)", name.c_str(), vertexPath.c_str(), fragmentPath.c_str());
        delete shader;
        return 0;
    }
    m_shaders[name] = shader;
    LogInfo("[ShaderManager] Loaded shader: %s (%s, %s)", name.c_str(), vertexPath.c_str(), fragmentPath.c_str());
    return shader;
}

Shader *ShaderManager::Create(const std::string &name, const char *vertexShader, const char *fragmentShader)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
        return it->second;
    Shader *shader = new Shader();
    if (!shader->Create(vertexShader, fragmentShader))
    {
        LogError("[ShaderManager] Failed to create shader: %s", name.c_str());
        delete shader;
        return 0;
    }
    m_shaders[name] = shader;
    LogInfo("[ShaderManager] Created shader: %s", name.c_str());
    return shader;
}

Shader *ShaderManager::Get(const std::string &name)
{
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second : nullptr;
}

void ShaderManager::Init()
{
    
        const char *vShader = GLSL(

            layout(location = 0) in vec3 position;
            layout(location = 1) in vec2 texCoord;
            layout(location = 2) in vec4 color;

            uniform mat4 mvp;

            out vec2 TexCoord;
            out vec4 vertexColor;
            void main() {
                gl_Position = mvp * vec4(position, 1.0);
                TexCoord = texCoord;
                vertexColor = color;
            });

        const char *fShader =
            GLSL(
                
                in vec2 TexCoord;
                out vec4 color;
                in vec4 vertexColor;
                uniform sampler2D texture0;
                void main() 
                {
                    color = texture(texture0, TexCoord) * vertexColor;
                });

        if (ShaderManager::Instance().Create("Batch",vShader, fShader))
        {
            Shader *shader = ShaderManager::Instance().Get("Batch");

            shader->Create(vShader, fShader);
            Logger::Instance().Info("[ShaderManager] Batch Shader Created");
        }
        else
        {
            Logger::Instance().Error("[ShaderManager]  Failed to create Batch Shader");
        }
    
}

void ShaderManager::UnloadAll()
{
    if (m_shaders.empty())
        return;
    for (auto &[name, shader] : m_shaders)
    {
        delete shader;
    }
    m_shaders.clear();
    LogInfo("[ShaderManager] Unloaded all shaders");
}