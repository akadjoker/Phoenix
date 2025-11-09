#include "pch.h"
#include "Driver.hpp"
#include "Utils.hpp"
#include "RenderTarget.hpp"
#include "Texture.hpp"
#include "glad/glad.h"

// ============================================
// OPENGL FORMAT CONVERSION HELPERS
// ============================================

extern u32 ToGLFormat(TextureFormat format);
extern GLenum GetDataFormat(TextureFormat format);
extern GLenum GetDataType(TextureFormat format);

static const char* FramebufferStatusString(GLenum status)
{
    switch (status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            return "Complete";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "Incomplete Attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "Missing Attachment";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "Unsupported Format Combination";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "Incomplete Multisample";
        default:
            return "Unknown Error";
    }
}

// ============================================
// DEVICE LIMITS CACHE (GLES 3.2)
// ============================================

struct GLESLimits
{
    static GLint maxDrawBuffers;
    static GLint maxSamples;
    static GLint maxArrayLayers;
    static GLint maxCubemapSize;
    static GLint maxRenderbufferSize;
    static bool initialized;
    
    static void Initialize()
    {
        if (initialized) return;
        
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubemapSize);
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
        
        LogInfo("[GLES] Device Limits:");
        LogInfo("  Max Draw Buffers: %d", maxDrawBuffers);
        LogInfo("  Max MSAA Samples: %d", maxSamples);
        LogInfo("  Max Array Layers: %d", maxArrayLayers);
        LogInfo("  Max Cubemap Size: %d", maxCubemapSize);
        LogInfo("  Max Renderbuffer Size: %d", maxRenderbufferSize);
        
        initialized = true;
    }
};

GLint GLESLimits::maxDrawBuffers = 4;
GLint GLESLimits::maxSamples = 4;
GLint GLESLimits::maxArrayLayers = 256;
GLint GLESLimits::maxCubemapSize = 2048;
GLint GLESLimits::maxRenderbufferSize = 4096;
bool GLESLimits::initialized = false;

// ============================================
// RENDER TARGET IMPLEMENTATION
// ============================================

RenderTarget::RenderTarget()
{
    GLESLimits::Initialize();
}

RenderTarget::~RenderTarget()
{
    Release();
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : m_fbo(other.m_fbo),
      m_width(other.m_width),
      m_height(other.m_height),
      m_isValid(other.m_isValid),
      m_isFinalized(other.m_isFinalized),
      m_colorAttachments(std::move(other.m_colorAttachments)),
      m_depthAttachment(other.m_depthAttachment),
      m_hasDepth(other.m_hasDepth),
      m_name(std::move(other.m_name))
{
    // CRÍTICO: Zera o other para evitar double-delete
    other.m_fbo = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_isValid = false;
    other.m_isFinalized = false;
    other.m_hasDepth = false;
    other.m_depthAttachment.texture = nullptr;
    other.m_depthAttachment.rbo = 0;
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
{
    if (this != &other)
    {
        Release();
        
        m_fbo = other.m_fbo;
        m_width = other.m_width;
        m_height = other.m_height;
        m_isValid = other.m_isValid;
        m_isFinalized = other.m_isFinalized;
        m_colorAttachments = std::move(other.m_colorAttachments);
        m_depthAttachment = other.m_depthAttachment;
        m_hasDepth = other.m_hasDepth;
        m_name = std::move(other.m_name);
        
        // CRÍTICO: Zera o other
        other.m_fbo = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_isValid = false;
        other.m_isFinalized = false;
        other.m_hasDepth = false;
        other.m_depthAttachment.texture = nullptr;
        other.m_depthAttachment.rbo = 0;
    }
    return *this;
}

bool RenderTarget::Create(u32 width, u32 height)
{
    if (m_isValid)
        Release();
    
    // Validate dimensions
    if (width == 0 || height == 0)
    {
        LogError("[RenderTarget:%s] Invalid dimensions: %ux%u", m_name.c_str(), width, height);
        return false;
    }
    
    // Check device limits
    if (width > (u32)GLESLimits::maxRenderbufferSize || 
        height > (u32)GLESLimits::maxRenderbufferSize)
    {
        LogError("[RenderTarget:%s] Dimensions %ux%u exceed device limit %d",
                 m_name.c_str(), width, height, GLESLimits::maxRenderbufferSize);
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // Create framebuffer object
    CHECK_GL_ERROR(glGenFramebuffers(1, &m_fbo));
    
    if (m_fbo == 0)
    {
        LogError("[RenderTarget:%s] Failed to generate framebuffer", m_name.c_str());
        return false;
    }
    
    m_isValid = true;
    return true;
}

u32 RenderTarget::AddColorAttachment(TextureFormat format)
{
    AttachmentConfig config;
    config.type = AttachmentType::COLOR;
    config.format = format;
    config.colorAttachmentIndex = (u32)m_colorAttachments.size();
    
    return AddColorAttachment(config);
}

u32 RenderTarget::AddColorAttachment(const AttachmentConfig& config)
{
    if (!m_isValid)
    {
        LogError("[RenderTarget:%s] Cannot add attachment to invalid render target", m_name.c_str());
        return 0;
    }
    
    if (m_isFinalized)
    {
        LogError("[RenderTarget:%s] Cannot add attachment after finalization", m_name.c_str());
        return 0;
    }
    
    // CRÍTICO: Valida limites GLES
    if (m_colorAttachments.size() >= (size_t)GLESLimits::maxDrawBuffers)
    {
        LogError("[RenderTarget:%s] Max color attachments reached (%d)", 
                 m_name.c_str(), GLESLimits::maxDrawBuffers);
        return 0;
    }
    
    ColorAttachment att;
    att.config = config;
    att.config.colorAttachmentIndex = (u32)m_colorAttachments.size();
    
    CreateColorAttachment(att, att.config.colorAttachmentIndex);
    
    m_colorAttachments.push_back(att);
    return att.config.colorAttachmentIndex;
}

bool RenderTarget::AddDepthAttachment(TextureFormat format)
{
    AttachmentConfig config;
    config.type = AttachmentType::DEPTH;
    config.format = format;
    
    return AddDepthAttachment(config);
}

bool RenderTarget::AddDepthAttachment(const AttachmentConfig& config)
{
    if (!m_isValid || m_hasDepth || m_isFinalized)
    {
        LogError("[RenderTarget:%s] Cannot add depth attachment", m_name.c_str());
        return false;
    }
    
    m_depthAttachment.config = config;
    m_depthAttachment.hasStencil = false;
    
    CreateDepthAttachment(m_depthAttachment);
    
    m_hasDepth = true;
    return true;
}

bool RenderTarget::AddDepthStencilAttachment()
{
    AttachmentConfig config;
    config.type = AttachmentType::DEPTH_STENCIL;
    config.format = TextureFormat::DEPTH24_STENCIL8;
    
    if (!m_isValid || m_hasDepth || m_isFinalized)
    {
        LogError("[RenderTarget:%s] Cannot add depth-stencil attachment", m_name.c_str());
        return false;
    }
    
    m_depthAttachment.config = config;
    m_depthAttachment.hasStencil = true;
    
    CreateDepthAttachment(m_depthAttachment);
    
    m_hasDepth = true;
    return true;
}

void RenderTarget::CreateColorAttachment(ColorAttachment& att, u32 attachmentIndex)
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    if (att.config.useRenderbuffer)
    {
        // Use Renderbuffer (cannot be sampled as texture)
        CHECK_GL_ERROR(glGenRenderbuffers(1, &att.rbo));
        CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, att.rbo));
        
        GLenum internalFormat = ToGLFormat(att.config.format);
        CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_width, m_height));
        
        CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex,
                                                  GL_RENDERBUFFER, att.rbo));
    }
    else
    {
        // Use Texture (can be sampled)
        att.texture = new Texture();
        
        if (!att.texture->Create(m_width, m_height, att.config.format, nullptr))
        {
            LogError("[RenderTarget:%s] Failed to create color texture for attachment %u",
                     m_name.c_str(), attachmentIndex);
            delete att.texture;
            att.texture = nullptr;
            CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            return;
        }
        
        att.texture->SetMinFilter(att.config.minFilter);
        att.texture->SetMagFilter(att.config.magFilter);
        att.texture->SetWrap(att.config.wrap);
        
        CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex,
                                              GL_TEXTURE_2D, att.texture->GetHandle(), 0));
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void RenderTarget::CreateDepthAttachment(DepthAttachment& att)
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    GLenum attachment = att.hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
    
    if (att.config.useRenderbuffer)
    {
        // Use Renderbuffer
        CHECK_GL_ERROR(glGenRenderbuffers(1, &att.rbo));
        CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, att.rbo));
        
        GLenum internalFormat = ToGLFormat(att.config.format);
        CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_width, m_height));
        
        CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment,
                                                  GL_RENDERBUFFER, att.rbo));
    }
    else
    {
        // Use Texture
        att.texture = new Texture();
        
        if (!att.texture->Create(m_width, m_height, att.config.format, nullptr))
        {
            LogError("[RenderTarget:%s] Failed to create depth texture", m_name.c_str());
            delete att.texture;
            att.texture = nullptr;
            CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            return;
        }
        
        att.texture->SetMinFilter(FilterMode::NEAREST);
        att.texture->SetMagFilter(FilterMode::NEAREST);
        att.texture->SetWrap(WrapMode::CLAMP_TO_EDGE);
        
        CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                                              GL_TEXTURE_2D, att.texture->GetHandle(), 0));
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

bool RenderTarget::Finalize()
{
    if (!m_isValid)
    {
        LogError("[RenderTarget:%s] Cannot finalize invalid render target", m_name.c_str());
        return false;
    }
    
    if (m_isFinalized)
    {
        LogWarning("[RenderTarget:%s] Already finalized", m_name.c_str());
        return true;
    }
    
    if (m_colorAttachments.empty() && !m_hasDepth)
    {
        LogError("[RenderTarget:%s] Must have at least one attachment", m_name.c_str());
        return false;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Update draw buffers for MRT
    if (!m_colorAttachments.empty())
    {
        UpdateDrawBuffers();
    }
    else
    {
        // Depth-only FBO
        GLenum none = GL_NONE;
        CHECK_GL_ERROR(glDrawBuffers(1, &none));
    }
    
    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("[RenderTarget:%s] Framebuffer incomplete: %s",
                 m_name.c_str(), FramebufferStatusString(status));
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        return false;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    m_isFinalized = true;
    LogInfo("[RenderTarget:%s] Finalized: %ux%u, %zu color attachments, depth: %s",
            m_name.c_str(), m_width, m_height, m_colorAttachments.size(),
            m_hasDepth ? "Yes" : "No");
    
    return true;
}

void RenderTarget::UpdateDrawBuffers()
{
    if (m_colorAttachments.empty())
        return;
    
    std::vector<GLenum> drawBuffers;
    drawBuffers.reserve(m_colorAttachments.size());
    
    for (size_t i = 0; i < m_colorAttachments.size(); ++i)
    {
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + (GLenum)i);
    }
    
    CHECK_GL_ERROR(glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));
}

void RenderTarget::Bind() const
{
    if (!m_isValid || !m_isFinalized)
    {
        LogError("[RenderTarget:%s] Cannot bind invalid or unfinalized render target", m_name.c_str());
        return;
    }
    Driver::Instance().SaveViewPort();
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    Driver::Instance().SetViewPort(0, 0, m_width, m_height);
}

void RenderTarget::Unbind() const
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    Driver::Instance().RestoreViewPort();
 
}

void RenderTarget::Clear(bool clearColor, bool clearDepth, bool clearStencil)
{
    GLbitfield mask = 0;
    if (clearColor) mask |= GL_COLOR_BUFFER_BIT;
    if (clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    if (clearStencil) mask |= GL_STENCIL_BUFFER_BIT;
    
    if (mask != 0)
    {
        CHECK_GL_ERROR(glClear(mask));
    }
}

void RenderTarget::ClearColor(float r, float g, float b, float a)
{
    CHECK_GL_ERROR(glClearColor(r, g, b, a));
    CHECK_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT));
}

void RenderTarget::ClearColorAttachment(u32 index, float r, float g, float b, float a)
{
    if (index >= m_colorAttachments.size())
    {
        LogError("[RenderTarget:%s] Invalid color attachment index %u", m_name.c_str(), index);
        return;
    }
    
    float clearColor[4] = {r, g, b, a};
    CHECK_GL_ERROR(glClearBufferfv(GL_COLOR, index, clearColor));
}

// CRÍTICO: Release com delete das texturas!
void RenderTarget::Release()
{
    if (m_fbo)
    {
        CHECK_GL_ERROR(glDeleteFramebuffers(1, &m_fbo));
        m_fbo = 0;
    }
    
    // CRÍTICO: Liberta as texturas de color!
    for (auto& att : m_colorAttachments)
    {
        if (att.texture)
        {
            delete att.texture;
            att.texture = nullptr;
        }
        if (att.rbo)
        {
            CHECK_GL_ERROR(glDeleteRenderbuffers(1, &att.rbo));
            att.rbo = 0;
        }
    }
    m_colorAttachments.clear();
    
    // CRÍTICO: Liberta a textura de depth!
    if (m_depthAttachment.texture)
    {
        delete m_depthAttachment.texture;
        m_depthAttachment.texture = nullptr;
    }
    if (m_depthAttachment.rbo)
    {
        CHECK_GL_ERROR(glDeleteRenderbuffers(1, &m_depthAttachment.rbo));
        m_depthAttachment.rbo = 0;
    }
    
    m_isValid = false;
    m_isFinalized = false;
    m_hasDepth = false;
    m_width = 0;
    m_height = 0;
}

bool RenderTarget::Resize(u32 newWidth, u32 newHeight)
{
    if (!m_isValid)
    {
        LogError("[RenderTarget:%s] Cannot resize invalid render target", m_name.c_str());
        return false;
    }
    
    if (newWidth == m_width && newHeight == m_height)
    {
        return true;
    }
    
    LogInfo("[RenderTarget:%s] Resizing from %ux%u to %ux%u",
            m_name.c_str(), m_width, m_height, newWidth, newHeight);
    
    // Save attachment configurations
    std::vector<AttachmentConfig> colorConfigs;
    for (const auto& att : m_colorAttachments)
    {
        colorConfigs.push_back(att.config);
    }
    
    AttachmentConfig depthConfig = m_depthAttachment.config;
    bool hadDepth = m_hasDepth;
    bool hadStencil = m_depthAttachment.hasStencil;
    
    // Release old resources
    Release();
    
    // Recreate with new size
    if (!Create(newWidth, newHeight))
    {
        return false;
    }
    
    // Recreate attachments
    for (const auto& config : colorConfigs)
    {
        AddColorAttachment(config);
    }
    
    if (hadDepth)
    {
        if (hadStencil)
        {
            AddDepthStencilAttachment();
        }
        else
        {
            AddDepthAttachment(depthConfig);
        }
    }
    
    return Finalize();
}

void RenderTarget::BlitTo(RenderTarget* target, bool blitColor, bool blitDepth)
{
    if (!target || !m_isValid || !target->IsValid())
    {
        LogError("[RenderTarget] Invalid blit operation");
        return;
    }
    
    GLbitfield mask = 0;
    if (blitColor) mask |= GL_COLOR_BUFFER_BIT;
    if (blitDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->GetFBO()));
    
    CHECK_GL_ERROR(glBlitFramebuffer(
        0, 0, m_width, m_height,
        0, 0, target->GetWidth(), target->GetHeight(),
        mask, GL_NEAREST
    ));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void RenderTarget::BlitToScreen(u32 screenWidth, u32 screenHeight, bool blitColor, bool blitDepth)
{
    if (!m_isValid)
    {
        LogError("[RenderTarget:%s] Cannot blit invalid render target", m_name.c_str());
        return;
    }
    
    GLbitfield mask = 0;
    if (blitColor) mask |= GL_COLOR_BUFFER_BIT;
    if (blitDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
    
    CHECK_GL_ERROR(glBlitFramebuffer(
        0, 0, m_width, m_height,
        0, 0, screenWidth, screenHeight,
        mask, GL_NEAREST
    ));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

Texture* RenderTarget::GetColorTexture(u32 index)
{
    if (index >= m_colorAttachments.size())
    {
        LogError("[RenderTarget:%s] Invalid color attachment index %u", m_name.c_str(), index);
        return nullptr;
    }
    
    return m_colorAttachments[index].texture;
}

Texture* RenderTarget::GetDepthTexture()
{
    return m_depthAttachment.texture;
}

u32 RenderTarget::GetColorTextureID(u32 index) const
{
    if (index >= m_colorAttachments.size())
    {
        LogError("[RenderTarget:%s] Invalid color attachment index %u", m_name.c_str(), index);
        return 0;
    }
    
    return m_colorAttachments[index].texture ? m_colorAttachments[index].texture->GetHandle() : 0;
}

u32 RenderTarget::GetDepthTextureID() const
{
    return m_depthAttachment.texture ? m_depthAttachment.texture->GetHandle() : 0;
}

bool RenderTarget::CheckStatus() const
{
    if (!m_isValid)
        return false;
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("[RenderTarget:%s] Framebuffer status: %s",
                 m_name.c_str(), FramebufferStatusString(status));
        return false;
    }
    
    return true;
}

void RenderTarget::PrintInfo() const
{
    LogInfo("========================================");
    LogInfo("  RenderTarget: %s", m_name.c_str());
    LogInfo("========================================");
    LogInfo("  Valid: %s", m_isValid ? "Yes" : "No");
    LogInfo("  Finalized: %s", m_isFinalized ? "Yes" : "No");
    LogInfo("  Size: %ux%u", m_width, m_height);
    LogInfo("  FBO Handle: %u", m_fbo);
    LogInfo("  Color Attachments: %zu", m_colorAttachments.size());
    
    for (size_t i = 0; i < m_colorAttachments.size(); ++i)
    {
        const auto& att = m_colorAttachments[i];
        LogInfo("    [%zu] Texture: %u, RBO: %u", i,
                att.texture ? att.texture->GetHandle() : 0, att.rbo);
    }
    
    if (m_hasDepth)
    {
        LogInfo("  Depth: Yes (Texture: %u, RBO: %u, Stencil: %s)",
                m_depthAttachment.texture ? m_depthAttachment.texture->GetHandle() : 0,
                m_depthAttachment.rbo,
                m_depthAttachment.hasStencil ? "Yes" : "No");
    }
    else
    {
        LogInfo("  Depth: No");
    }
    
    LogInfo("========================================");
}

// ============================================
// MSAA RENDER TARGET IMPLEMENTATION
// ============================================

MSAARenderTarget::MSAARenderTarget()
{
    GLESLimits::Initialize();
}

MSAARenderTarget::~MSAARenderTarget()
{
    Release();
}

MSAARenderTarget::MSAARenderTarget(MSAARenderTarget&& other) noexcept
    : m_fbo(other.m_fbo),
      m_width(other.m_width),
      m_height(other.m_height),
      m_samples(other.m_samples),
      m_colorRBOs(std::move(other.m_colorRBOs)),
      m_depthRBO(other.m_depthRBO),
      m_isValid(other.m_isValid),
      m_isFinalized(other.m_isFinalized),
      m_name(std::move(other.m_name))
{
    // CRÍTICO: Zera o other
    other.m_fbo = 0;
    other.m_depthRBO = 0;
    other.m_isValid = false;
    other.m_isFinalized = false;
}

MSAARenderTarget& MSAARenderTarget::operator=(MSAARenderTarget&& other) noexcept
{
    if (this != &other)
    {
        Release();
        
        m_fbo = other.m_fbo;
        m_width = other.m_width;
        m_height = other.m_height;
        m_samples = other.m_samples;
        m_colorRBOs = std::move(other.m_colorRBOs);
        m_depthRBO = other.m_depthRBO;
        m_isValid = other.m_isValid;
        m_isFinalized = other.m_isFinalized;
        m_name = std::move(other.m_name);
        
        // CRÍTICO: Zera o other
        other.m_fbo = 0;
        other.m_depthRBO = 0;
        other.m_isValid = false;
        other.m_isFinalized = false;
    }
    return *this;
}

bool MSAARenderTarget::Create(u32 width, u32 height, u32 samples)
{
    if (m_isValid)
        Release();
    
    if (width == 0 || height == 0)
    {
        LogError("[MSAA:%s] Invalid dimensions: %ux%u", m_name.c_str(), width, height);
        return false;
    }
    
    // CRÍTICO: Valida e clamp samples ao máximo do device
    if (samples > (u32)GLESLimits::maxSamples)
    {
        LogWarning("[MSAA:%s] Requested %u samples, clamping to device max %d",
                   m_name.c_str(), samples, GLESLimits::maxSamples);
        samples = GLESLimits::maxSamples;
    }
    
    // Garante que samples é potência de 2 e >= 1
    if (samples < 1) samples = 1;
    if (samples > 1 && (samples & (samples - 1)) != 0)
    {
        // Arredonda para próxima potência de 2
        u32 pow2 = 1;
        while (pow2 < samples && pow2 < (u32)GLESLimits::maxSamples)
            pow2 <<= 1;
        samples = pow2;
    }
    
    m_width = width;
    m_height = height;
    m_samples = samples;
    
    CHECK_GL_ERROR(glGenFramebuffers(1, &m_fbo));
    if (m_fbo == 0)
    {
        LogError("[MSAA:%s] Failed to generate framebuffer", m_name.c_str());
        return false;
    }
    
    m_isValid = true;
    LogInfo("[MSAA:%s] Created %ux%u with %ux samples", m_name.c_str(), width, height, samples);
    return true;
}

u32 MSAARenderTarget::AddColorAttachment(TextureFormat format)
{
    if (!m_isValid || m_isFinalized)
    {
        LogError("[MSAA:%s] Cannot add attachment", m_name.c_str());
        return 0;
    }
    
    // CRÍTICO: Valida limites
    if (m_colorRBOs.size() >= (size_t)GLESLimits::maxDrawBuffers)
    {
        LogError("[MSAA:%s] Max color attachments reached (%d)",
                 m_name.c_str(), GLESLimits::maxDrawBuffers);
        return 0;
    }
    
    u32 rbo = 0;
    CHECK_GL_ERROR(glGenRenderbuffers(1, &rbo));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    
    GLenum internalFormat = ToGLFormat(format);
    
    // MSAA renderbuffer storage
    CHECK_GL_ERROR(glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, m_samples, internalFormat, m_width, m_height
    ));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    u32 attachmentIndex = (u32)m_colorRBOs.size();
    CHECK_GL_ERROR(glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex, GL_RENDERBUFFER, rbo
    ));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    m_colorRBOs.push_back(rbo);
    return attachmentIndex;
}

bool MSAARenderTarget::AddDepthAttachment(TextureFormat format)
{
    if (!m_isValid || m_depthRBO != 0 || m_isFinalized)
    {
        LogError("[MSAA:%s] Cannot add depth attachment", m_name.c_str());
        return false;
    }
    
    CHECK_GL_ERROR(glGenRenderbuffers(1, &m_depthRBO));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO));
    
    GLenum internalFormat = ToGLFormat(format);
    
    // MSAA depth renderbuffer storage
    CHECK_GL_ERROR(glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, m_samples, internalFormat, m_width, m_height
    ));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Depth ou depth+stencil?
    GLenum attachment = (format == TextureFormat::DEPTH24_STENCIL8) ? 
                        GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
    
    CHECK_GL_ERROR(glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_depthRBO
    ));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    return true;
}

bool MSAARenderTarget::Finalize()
{
    if (!m_isValid)
    {
        LogError("[MSAA:%s] Cannot finalize invalid target", m_name.c_str());
        return false;
    }
    
    if (m_isFinalized)
    {
        return true;
    }
    
    if (m_colorRBOs.empty())
    {
        LogError("[MSAA:%s] Must have at least one color attachment", m_name.c_str());
        return false;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Setup draw buffers
    std::vector<GLenum> drawBuffers;
    for (size_t i = 0; i < m_colorRBOs.size(); ++i)
    {
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + (GLenum)i);
    }
    CHECK_GL_ERROR(glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));
    
    // Check status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("[MSAA:%s] Framebuffer incomplete: %s",
                 m_name.c_str(), FramebufferStatusString(status));
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        return false;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    m_isFinalized = true;
    LogInfo("[MSAA:%s] Finalized: %ux%u, %ux samples, %zu attachments",
            m_name.c_str(), m_width, m_height, m_samples, m_colorRBOs.size());
    return true;
}

void MSAARenderTarget::Bind() const
{
    if (!m_isValid || !m_isFinalized)
    {
        LogError("[MSAA:%s] Cannot bind invalid/unfinalized target", m_name.c_str());
        return;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    CHECK_GL_ERROR(glViewport(0, 0, m_width, m_height));
}

void MSAARenderTarget::Unbind() const
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void MSAARenderTarget::Clear(bool clearColor, bool clearDepth)
{
    GLbitfield mask = 0;
    if (clearColor) mask |= GL_COLOR_BUFFER_BIT;
    if (clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    if (mask != 0)
    {
        CHECK_GL_ERROR(glClear(mask));
    }
}

void MSAARenderTarget::ResolveTo(RenderTarget* target, bool color, bool depth)
{
    if (!target || !m_isValid || !target->IsValid())
    {
        LogError("[MSAA:%s] Invalid resolve operation", m_name.c_str());
        return;
    }
    
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->GetFBO()));
    
    CHECK_GL_ERROR(glBlitFramebuffer(
        0, 0, m_width, m_height,
        0, 0, target->GetWidth(), target->GetHeight(),
        mask, GL_NEAREST
    ));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void MSAARenderTarget::Release()
{
    if (m_fbo)
    {
        CHECK_GL_ERROR(glDeleteFramebuffers(1, &m_fbo));
        m_fbo = 0;
    }
    
    for (u32 rbo : m_colorRBOs)
    {
        if (rbo)
        {
            CHECK_GL_ERROR(glDeleteRenderbuffers(1, &rbo));
        }
    }
    m_colorRBOs.clear();
    
    if (m_depthRBO)
    {
        CHECK_GL_ERROR(glDeleteRenderbuffers(1, &m_depthRBO));
        m_depthRBO = 0;
    }
    
    m_isValid = false;
    m_isFinalized = false;
}

// ============================================
// CUBEMAP RENDER TARGET IMPLEMENTATION
// ============================================

CubemapRenderTarget::CubemapRenderTarget()
{
    GLESLimits::Initialize();
}

CubemapRenderTarget::~CubemapRenderTarget()
{
    Release();
}

CubemapRenderTarget::CubemapRenderTarget(CubemapRenderTarget&& other) noexcept
    : m_fbo(other.m_fbo),
      m_size(other.m_size),
      m_cubemap(other.m_cubemap),
      m_depthRBO(other.m_depthRBO),
      m_isValid(other.m_isValid),
      m_name(std::move(other.m_name))
{
    // CRÍTICO: Zera o other
    other.m_fbo = 0;
    other.m_cubemap = nullptr;
    other.m_depthRBO = 0;
    other.m_isValid = false;
}

CubemapRenderTarget& CubemapRenderTarget::operator=(CubemapRenderTarget&& other) noexcept
{
    if (this != &other)
    {
        Release();
        
        m_fbo = other.m_fbo;
        m_size = other.m_size;
        m_cubemap = other.m_cubemap;
        m_depthRBO = other.m_depthRBO;
        m_isValid = other.m_isValid;
        m_name = std::move(other.m_name);
        
        // CRÍTICO: Zera o other
        other.m_fbo = 0;
        other.m_cubemap = nullptr;
        other.m_depthRBO = 0;
        other.m_isValid = false;
    }
    return *this;
}

bool CubemapRenderTarget::Create(u32 size, TextureFormat colorFormat, bool withDepth)
{
    if (m_isValid)
        Release();
    
    if (size == 0)
    {
        LogError("[Cubemap:%s] Invalid size: %u", m_name.c_str(), size);
        return false;
    }
    
    // CRÍTICO: Valida limite do device
    if (size > (u32)GLESLimits::maxCubemapSize)
    {
        LogError("[Cubemap:%s] Size %u exceeds device limit %d",
                 m_name.c_str(), size, GLESLimits::maxCubemapSize);
        return false;
    }
    
    m_size = size;
    
    // Create cubemap texture
    m_cubemap = new Texture();
    if (!m_cubemap->CreateCube(size, colorFormat, nullptr))
    {
        LogError("[Cubemap:%s] Failed to create cubemap texture", m_name.c_str());
        delete m_cubemap;
        m_cubemap = nullptr;
        return false;
    }
    
    m_cubemap->SetMinFilter(FilterMode::LINEAR);
    m_cubemap->SetMagFilter(FilterMode::LINEAR);
    m_cubemap->SetWrap(WrapMode::CLAMP_TO_EDGE);
    
    // Create FBO
    CHECK_GL_ERROR(glGenFramebuffers(1, &m_fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Create depth renderbuffer (shared across all faces)
    if (withDepth)
    {
        CHECK_GL_ERROR(glGenRenderbuffers(1, &m_depthRBO));
        CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO));
        CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size));
        CHECK_GL_ERROR(glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRBO
        ));
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    m_isValid = true;
    LogInfo("[Cubemap:%s] Created %ux%u, depth: %s",
            m_name.c_str(), size, size, withDepth ? "Yes" : "No");
    return true;
}

void CubemapRenderTarget::BindFace(u32 face)
{
    if (!m_isValid || face >= 6)
    {
        LogError("[Cubemap:%s] Cannot bind face %u", m_name.c_str(), face);
        return;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Attach specific cubemap face
    CHECK_GL_ERROR(glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
        m_cubemap->GetHandle(),
        0
    ));
    
    // Verify framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("[Cubemap:%s] Framebuffer incomplete for face %u: %s",
                 m_name.c_str(), face, FramebufferStatusString(status));
    }
    
    CHECK_GL_ERROR(glViewport(0, 0, m_size, m_size));
}

void CubemapRenderTarget::Unbind() const
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void CubemapRenderTarget::Clear(bool clearColor, bool clearDepth)
{
    GLbitfield mask = 0;
    if (clearColor) mask |= GL_COLOR_BUFFER_BIT;
    if (clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    if (mask != 0)
    {
        CHECK_GL_ERROR(glClear(mask));
    }
}

u32 CubemapRenderTarget::GetCubemapTextureID() const
{
    return m_cubemap ? m_cubemap->GetHandle() : 0;
}

void CubemapRenderTarget::Release()
{
    if (m_fbo)
    {
        CHECK_GL_ERROR(glDeleteFramebuffers(1, &m_fbo));
        m_fbo = 0;
    }
    
    // CRÍTICO: Delete cubemap texture!
    if (m_cubemap)
    {
        delete m_cubemap;
        m_cubemap = nullptr;
    }
    
    if (m_depthRBO)
    {
        CHECK_GL_ERROR(glDeleteRenderbuffers(1, &m_depthRBO));
        m_depthRBO = 0;
    }
    
    m_isValid = false;
}

// ============================================
// ARRAY RENDER TARGET IMPLEMENTATION
// ============================================

ArrayRenderTarget::ArrayRenderTarget()
{
    GLESLimits::Initialize();
}

ArrayRenderTarget::~ArrayRenderTarget()
{
    Release();
}

ArrayRenderTarget::ArrayRenderTarget(ArrayRenderTarget&& other) noexcept
    : m_fbo(other.m_fbo),
      m_width(other.m_width),
      m_height(other.m_height),
      m_layers(other.m_layers),
      m_arrayTexture(other.m_arrayTexture),
      m_isDepthOnly(other.m_isDepthOnly),
      m_isValid(other.m_isValid),
      m_name(std::move(other.m_name))
{
    // CRÍTICO: Zera o other
    other.m_fbo = 0;
    other.m_arrayTexture = nullptr;
    other.m_isValid = false;
}

ArrayRenderTarget& ArrayRenderTarget::operator=(ArrayRenderTarget&& other) noexcept
{
    if (this != &other)
    {
        Release();
        
        m_fbo = other.m_fbo;
        m_width = other.m_width;
        m_height = other.m_height;
        m_layers = other.m_layers;
        m_arrayTexture = other.m_arrayTexture;
        m_isDepthOnly = other.m_isDepthOnly;
        m_isValid = other.m_isValid;
        m_name = std::move(other.m_name);
        
        // CRÍTICO: Zera o other
        other.m_fbo = 0;
        other.m_arrayTexture = nullptr;
        other.m_isValid = false;
    }
    return *this;
}

bool ArrayRenderTarget::CreateDepthArray(u32 width, u32 height, u32 layers)
{
    if (m_isValid)
        Release();
    
    if (width == 0 || height == 0 || layers == 0)
    {
        LogError("[Array:%s] Invalid parameters: %ux%ux%u", m_name.c_str(), width, height, layers);
        return false;
    }
    
    // CRÍTICO: Valida limites
    if (layers > (u32)GLESLimits::maxArrayLayers)
    {
        LogError("[Array:%s] Layers %u exceeds device limit %d",
                 m_name.c_str(), layers, GLESLimits::maxArrayLayers);
        return false;
    }
    
    m_width = width;
    m_height = height;
    m_layers = layers;
    m_isDepthOnly = true;
    
    // Create 2D array depth texture
    // NOTA: Texture::Create3D pode ser usado se tiveres suporte para texture arrays
    // Senão precisas criar manualmente:
    m_arrayTexture = new Texture();
    
    // Cria texture 2D array manualmente (assumindo que tens GL_TEXTURE_2D_ARRAY support)
    u32 texHandle = 0;
    CHECK_GL_ERROR(glGenTextures(1, &texHandle));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D_ARRAY, texHandle));
    
    // Allocate storage para todas as layers
    CHECK_GL_ERROR(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, 
                                  width, height, layers));
    
    // Setup texture parameters
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    
    // HACK: Store handle directly (adapta ao teu sistema)
    // Idealmente a Texture class deveria suportar arrays nativamente
    
    // Create FBO
    CHECK_GL_ERROR(glGenFramebuffers(1, &m_fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Disable color attachment for depth-only
    GLenum none = GL_NONE;
    CHECK_GL_ERROR(glDrawBuffers(1, &none));
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    m_isValid = true;
    LogInfo("[Array:%s] Created depth array %ux%ux%u", m_name.c_str(), width, height, layers);
    return true;
}

bool ArrayRenderTarget::CreateColorArray(u32 width, u32 height, u32 layers, TextureFormat format)
{
    if (m_isValid)
        Release();
    
    if (width == 0 || height == 0 || layers == 0)
    {
        LogError("[Array:%s] Invalid parameters: %ux%ux%u", m_name.c_str(), width, height, layers);
        return false;
    }
    
    // CRÍTICO: Valida limites
    if (layers > (u32)GLESLimits::maxArrayLayers)
    {
        LogError("[Array:%s] Layers %u exceeds device limit %d",
                 m_name.c_str(), layers, GLESLimits::maxArrayLayers);
        return false;
    }
    
    m_width = width;
    m_height = height;
    m_layers = layers;
    m_isDepthOnly = false;
    
    // Create 2D array color texture
    m_arrayTexture = new Texture();
    
    u32 texHandle = 0;
    CHECK_GL_ERROR(glGenTextures(1, &texHandle));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D_ARRAY, texHandle));
    
    GLenum internalFormat = ToGLFormat(format);
    CHECK_GL_ERROR(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, 
                                  width, height, layers));
    
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    
    // Create FBO
    CHECK_GL_ERROR(glGenFramebuffers(1, &m_fbo));
    
    m_isValid = true;
    LogInfo("[Array:%s] Created color array %ux%ux%u", m_name.c_str(), width, height, layers);
    return true;
}

void ArrayRenderTarget::BindLayer(u32 layer)
{
    if (!m_isValid || layer >= m_layers)
    {
        LogError("[Array:%s] Cannot bind layer %u (max %u)", m_name.c_str(), layer, m_layers);
        return;
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    
    // Attach specific layer
    if (m_arrayTexture)
    {
        GLenum attachment = m_isDepthOnly ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;
        
        CHECK_GL_ERROR(glFramebufferTextureLayer(
            GL_FRAMEBUFFER,
            attachment,
            m_arrayTexture->GetHandle(),
            0,      // mipmap level
            layer   // array layer
        ));
    }
    
    CHECK_GL_ERROR(glViewport(0, 0, m_width, m_height));
}

void ArrayRenderTarget::Unbind() const
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void ArrayRenderTarget::Clear(bool clearColor, bool clearDepth)
{
    GLbitfield mask = 0;
    if (clearColor && !m_isDepthOnly) mask |= GL_COLOR_BUFFER_BIT;
    if (clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    if (mask != 0)
    {
        CHECK_GL_ERROR(glClear(mask));
    }
}

void ArrayRenderTarget::Release()
{
    if (m_fbo)
    {
        CHECK_GL_ERROR(glDeleteFramebuffers(1, &m_fbo));
        m_fbo = 0;
    }
    
    // CRÍTICO: Delete array texture!
    if (m_arrayTexture)
    {
        delete m_arrayTexture;
        m_arrayTexture = nullptr;
    }
    
    m_isValid = false;
}
// ============================================
// RENDER TARGET BUILDER IMPLEMENTATION
// ============================================

RenderTargetBuilder::RenderTargetBuilder(u32 width, u32 height)
    : m_width(width), m_height(height), m_name("Unnamed")
{
}

RenderTargetBuilder& RenderTargetBuilder::AddColorRGBA8()
{
    return AddColor(TextureFormat::RGBA8);
}

RenderTargetBuilder& RenderTargetBuilder::AddColorRGBA16F()
{
    return AddColor(TextureFormat::RGBA16F);
}

RenderTargetBuilder& RenderTargetBuilder::AddColorRGBA32F()
{
    return AddColor(TextureFormat::RGBA32F);
}

RenderTargetBuilder& RenderTargetBuilder::AddColorR8()
{
    return AddColor(TextureFormat::R8);
}

RenderTargetBuilder& RenderTargetBuilder::AddColorR16F()
{
    return AddColor(TextureFormat::R16F);
}

RenderTargetBuilder& RenderTargetBuilder::AddColorR32F()
{
    return AddColor(TextureFormat::R32F);
}

RenderTargetBuilder& RenderTargetBuilder::AddColor(TextureFormat format, FilterMode filter)
{
    AttachmentConfig config;
    config.type = AttachmentType::COLOR;
    config.format = format;
    config.minFilter = filter;
    config.magFilter = filter;
    config.wrap = WrapMode::CLAMP_TO_EDGE;
    
    m_colorConfigs.push_back(config);
    return *this;
}

RenderTargetBuilder& RenderTargetBuilder::AddColor(const AttachmentConfig& config)
{
    m_colorConfigs.push_back(config);
    return *this;
}

RenderTargetBuilder& RenderTargetBuilder::WithDepth(TextureFormat format)
{
    m_depthConfig.type = AttachmentType::DEPTH;
    m_depthConfig.format = format;
    m_hasDepth = true;
    m_hasDepthStencil = false;
    return *this;
}

RenderTargetBuilder& RenderTargetBuilder::WithDepthStencil()
{
    m_depthConfig.type = AttachmentType::DEPTH_STENCIL;
    m_depthConfig.format = TextureFormat::DEPTH24_STENCIL8;
    m_hasDepth = true;
    m_hasDepthStencil = true;
    return *this;
}

RenderTargetBuilder& RenderTargetBuilder::SetName(const std::string& name)
{
    m_name = name;
    return *this;
}

RenderTarget* RenderTargetBuilder::Build()
{
    RenderTarget* rt = new RenderTarget();
    rt->SetName(m_name);
    
    if (!rt->Create(m_width, m_height))
    {
        delete rt;
        return nullptr;
    }
    
    // Add color attachments
    for (const auto& config : m_colorConfigs)
    {
        rt->AddColorAttachment(config);
    }
    
    // Add depth attachment
    if (m_hasDepth)
    {
        if (m_hasDepthStencil)
        {
            rt->AddDepthStencilAttachment();
        }
        else
        {
            rt->AddDepthAttachment(m_depthConfig);
        }
    }
    
    // Finalize
    if (!rt->Finalize())
    {
        delete rt;
        return nullptr;
    }
    
    return rt;
}

// ============================================
// RENDER TARGET MANAGER IMPLEMENTATION
// ============================================

RenderTargetManager& RenderTargetManager::Instance()
{
    static RenderTargetManager instance;
    return instance;
}

RenderTargetManager::~RenderTargetManager()
{
     
}

// === DEVICE LIMITS (GLES 3.2) ===

u32 RenderTargetManager::GetMaxColorAttachments()
{
    GLESLimits::Initialize();
    return GLESLimits::maxDrawBuffers;
}

u32 RenderTargetManager::GetMaxSamples()
{
    GLESLimits::Initialize();
    return GLESLimits::maxSamples;
}

u32 RenderTargetManager::GetMaxArrayLayers()
{
    GLESLimits::Initialize();
    return GLESLimits::maxArrayLayers;
}

u32 RenderTargetManager::GetMaxCubemapSize()
{
    GLESLimits::Initialize();
    return GLESLimits::maxCubemapSize;
}

u32 RenderTargetManager::GetMaxRenderTargetSize()
{
    GLESLimits::Initialize();
    return GLESLimits::maxRenderbufferSize;
}

// === GENERIC CREATION ===

RenderTarget* RenderTargetManager::Create(const std::string& name, u32 width, u32 height)
{
    if (Exists(name))
    {
        LogWarning("[RTManager] RenderTarget '%s' already exists", name.c_str());
        return Get(name);
    }
    
    RenderTarget* rt = new RenderTarget();
    rt->SetName(name);
    
    if (!rt->Create(width, height))
    {
        LogError("[RTManager] Failed to create RenderTarget '%s'", name.c_str());
        delete rt;
        return nullptr;
    }
    
    m_targets[name] = rt;
    LogInfo("[RTManager] Created RenderTarget '%s'", name.c_str());
    return rt;
}

MSAARenderTarget* RenderTargetManager::CreateMSAA(const std::string& name, u32 width, u32 height, u32 samples)
{
    if (Exists(name))
    {
        LogWarning("[RTManager] MSAA RenderTarget '%s' already exists", name.c_str());
        return GetMSAA(name);
    }
    
    MSAARenderTarget* rt = new MSAARenderTarget();
    rt->SetName(name);
    
    if (!rt->Create(width, height, samples))
    {
        LogError("[RTManager] Failed to create MSAA RenderTarget '%s'", name.c_str());
        delete rt;
        return nullptr;
    }
    
    m_msaaTargets[name] = rt;
    LogInfo("[RTManager] Created MSAA RenderTarget '%s'", name.c_str());
    return rt;
}

CubemapRenderTarget* RenderTargetManager::CreateCubemap(const std::string& name, u32 size)
{
    if (Exists(name))
    {
        LogWarning("[RTManager] Cubemap RenderTarget '%s' already exists", name.c_str());
        return GetCubemap(name);
    }
    
    CubemapRenderTarget* rt = new CubemapRenderTarget();
    rt->SetName(name);
    
    if (!rt->Create(size, TextureFormat::RGBA8, true))
    {
        LogError("[RTManager] Failed to create Cubemap RenderTarget '%s'", name.c_str());
        delete rt;
        return nullptr;
    }
    
    m_cubemapTargets[name] = rt;
    LogInfo("[RTManager] Created Cubemap RenderTarget '%s'", name.c_str());
    return rt;
}

ArrayRenderTarget* RenderTargetManager::CreateArray(const std::string& name, u32 width, u32 height, u32 layers)
{
    if (Exists(name))
    {
        LogWarning("[RTManager] Array RenderTarget '%s' already exists", name.c_str());
        return GetArray(name);
    }
    
    ArrayRenderTarget* rt = new ArrayRenderTarget();
    rt->SetName(name);
    
    if (!rt->CreateColorArray(width, height, layers, TextureFormat::RGBA8))
    {
        LogError("[RTManager] Failed to create Array RenderTarget '%s'", name.c_str());
        delete rt;
        return nullptr;
    }
    
    m_arrayTargets[name] = rt;
    LogInfo("[RTManager] Created Array RenderTarget '%s'", name.c_str());
    return rt;
}

// === FACTORY METHODS ===

RenderTarget* RenderTargetManager::CreateShadowMap(const std::string& name, u32 resolution)
{
    RenderTarget* rt = Create(name, resolution, resolution);
    if (!rt) return nullptr;
    
    rt->AddDepthAttachment(TextureFormat::DEPTH24);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    // Configure depth texture for shadow sampling
    if (Texture* depthTex = rt->GetDepthTexture())
    {
        depthTex->SetMinFilter(FilterMode::LINEAR);
        depthTex->SetMagFilter(FilterMode::LINEAR);
        depthTex->SetWrap(WrapMode::CLAMP_TO_EDGE);
    }
    
    return rt;
}

ArrayRenderTarget* RenderTargetManager::CreateCascadedShadowMap(const std::string& name, u32 resolution, u32 cascades)
{
    if (Exists(name))
    {
        Remove(name);
    }
    
    ArrayRenderTarget* rt = new ArrayRenderTarget();
    rt->SetName(name);
    
    if (!rt->CreateDepthArray(resolution, resolution, cascades))
    {
        delete rt;
        return nullptr;
    }
    
    m_arrayTargets[name] = rt;
    LogInfo("[RTManager] Created Cascaded Shadow Map '%s': %ux%u, %u cascades",
            name.c_str(), resolution, resolution, cascades);
    return rt;
}

RenderTarget* RenderTargetManager::CreateHDR(const std::string& name, u32 width, u32 height)
{
    RenderTarget* rt = Create(name, width, height);
    if (!rt) return nullptr;
    
    rt->AddColorAttachment(TextureFormat::RGBA16F);
    rt->AddDepthAttachment(TextureFormat::DEPTH24);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    return rt;
}

RenderTarget* RenderTargetManager::CreateGBuffer(const std::string& name, u32 width, u32 height)
{
    RenderTarget* rt = Create(name, width, height);
    if (!rt) return nullptr;
    
    // Position (RGBA16F) - RGB = position, A = unused/specular
    rt->AddColorAttachment(TextureFormat::RGBA16F);
    
    // Normal (RGBA16F) - RGB = normal, A = unused
    rt->AddColorAttachment(TextureFormat::RGBA16F);
    
    // Albedo + Specular (RGBA8) - RGB = albedo, A = specular
    rt->AddColorAttachment(TextureFormat::RGBA8);
    
    // Depth
    rt->AddDepthAttachment(TextureFormat::DEPTH24);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    return rt;
}

RenderTarget* RenderTargetManager::CreateSSAO(const std::string& name, u32 width, u32 height)
{
    RenderTarget* rt = Create(name, width, height);
    if (!rt) return nullptr;
    
    rt->AddColorAttachment(TextureFormat::R32F);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    // Configure for nearest filtering (typical for SSAO)
    if (Texture* tex = rt->GetColorTexture(0))
    {
        tex->SetMinFilter(FilterMode::NEAREST);
        tex->SetMagFilter(FilterMode::NEAREST);
    }
    
    return rt;
}

RenderTarget* RenderTargetManager::CreateBloom(const std::string& name, u32 width, u32 height)
{
    RenderTarget* rt = Create(name, width, height);
    if (!rt) return nullptr;
    
    rt->AddColorAttachment(TextureFormat::RGBA16F);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    // Configure for linear filtering (blur needs it)
    if (Texture* tex = rt->GetColorTexture(0))
    {
        tex->SetMinFilter(FilterMode::LINEAR);
        tex->SetMagFilter(FilterMode::LINEAR);
        tex->SetWrap(WrapMode::CLAMP_TO_EDGE);
    }
    
    return rt;
}

RenderTarget* RenderTargetManager::CreateWaterReflection(const std::string& name, u32 resolution)
{
    RenderTarget* rt = Create(name, resolution, resolution);
    if (!rt) return nullptr;
    
    rt->AddColorAttachment(TextureFormat::RGBA8);
    rt->AddDepthAttachment(TextureFormat::DEPTH24);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    return rt;
}

RenderTarget* RenderTargetManager::CreateWaterRefraction(const std::string& name, u32 resolution)
{
    RenderTarget* rt = Create(name, resolution, resolution);
    if (!rt) return nullptr;
    
    rt->AddColorAttachment(TextureFormat::RGBA8);
    rt->AddDepthAttachment(TextureFormat::DEPTH24);
    
    if (!rt->Finalize())
    {
        Remove(name);
        return nullptr;
    }
    
    return rt;
}

CubemapRenderTarget* RenderTargetManager::CreateEnvironmentMap(const std::string& name, u32 size)
{
    return CreateCubemap(name, size);
}

// === RETRIEVAL ===

RenderTarget* RenderTargetManager::Get(const std::string& name)
{
    auto it = m_targets.find(name);
    if (it != m_targets.end())
        return it->second;
    
    LogWarning("[RTManager] RenderTarget '%s' not found", name.c_str());
    return nullptr;
}

MSAARenderTarget* RenderTargetManager::GetMSAA(const std::string& name)
{
    auto it = m_msaaTargets.find(name);
    if (it != m_msaaTargets.end())
        return it->second;
    
    LogWarning("[RTManager] MSAA RenderTarget '%s' not found", name.c_str());
    return nullptr;
}

CubemapRenderTarget* RenderTargetManager::GetCubemap(const std::string& name)
{
    auto it = m_cubemapTargets.find(name);
    if (it != m_cubemapTargets.end())
        return it->second;
    
    LogWarning("[RTManager] Cubemap RenderTarget '%s' not found", name.c_str());
    return nullptr;
}

ArrayRenderTarget* RenderTargetManager::GetArray(const std::string& name)
{
    auto it = m_arrayTargets.find(name);
    if (it != m_arrayTargets.end())
        return it->second;
    
    LogWarning("[RTManager] Array RenderTarget '%s' not found", name.c_str());
    return nullptr;
}

// === REMOVAL (COM DELETES CORRIGIDOS!) ===

void RenderTargetManager::Remove(const std::string& name)
{
    // Try to find and remove from all target types
    auto it = m_targets.find(name);
    if (it != m_targets.end())
    {
        LogInfo("[RTManager] Removing RenderTarget '%s'", name.c_str());
        delete it->second;  // CRÍTICO!
        m_targets.erase(it);
        return;
    }
    
    auto it2 = m_msaaTargets.find(name);
    if (it2 != m_msaaTargets.end())
    {
        LogInfo("[RTManager] Removing MSAA RenderTarget '%s'", name.c_str());
        delete it2->second;  // CRÍTICO!
        m_msaaTargets.erase(it2);
        return;
    }
    
    auto it3 = m_cubemapTargets.find(name);
    if (it3 != m_cubemapTargets.end())
    {
        LogInfo("[RTManager] Removing Cubemap RenderTarget '%s'", name.c_str());
        delete it3->second;  // CRÍTICO!
        m_cubemapTargets.erase(it3);
        return;
    }
    
    auto it4 = m_arrayTargets.find(name);
    if (it4 != m_arrayTargets.end())
    {
        LogInfo("[RTManager] Removing Array RenderTarget '%s'", name.c_str());
        delete it4->second;  // CRÍTICO!
        m_arrayTargets.erase(it4);
        return;
    }
    
    LogWarning("[RTManager] RenderTarget '%s' not found for removal", name.c_str());
}

void RenderTargetManager::UnloadAll()
{
    LogInfo("[RTManager] Cleaning up all render targets...");
    
    // Clean up normal render targets
    for (auto& pair : m_targets)
    {
        LogInfo("[RTManager] Deleting RenderTarget '%s'", pair.first.c_str());
        delete pair.second;  // CRÍTICO!
    }
    m_targets.clear();
    
    // Clean up MSAA targets
    for (auto& pair : m_msaaTargets)
    {
        LogInfo("[RTManager] Deleting MSAA RenderTarget '%s'", pair.first.c_str());
        delete pair.second;  // CRÍTICO!
    }
    m_msaaTargets.clear();
    
    // Clean up cubemap targets
    for (auto& pair : m_cubemapTargets)
    {
        LogInfo("[RTManager] Deleting Cubemap RenderTarget '%s'", pair.first.c_str());
        delete pair.second;  // CRÍTICO!
    }
    m_cubemapTargets.clear();
    
    // Clean up array targets
    for (auto& pair : m_arrayTargets)
    {
        LogInfo("[RTManager] Deleting Array RenderTarget '%s'", pair.first.c_str());
        delete pair.second;  // CRÍTICO!
    }
    m_arrayTargets.clear();
    
    LogInfo("[RTManager] All render targets cleaned up successfully");
}

// === STATISTICS ===

size_t RenderTargetManager::GetCount() const
{
    return m_targets.size() + m_msaaTargets.size() + 
           m_cubemapTargets.size() + m_arrayTargets.size();
}

size_t RenderTargetManager::GetMemoryUsage() const
{
    size_t totalBytes = 0;
    
    // Estimate memory for normal render targets
    for (const auto& pair : m_targets)
    {
        auto rt = pair.second;
        if (!rt->IsValid()) continue;
        
        // Color attachments (approximate 4 bytes per pixel per attachment)
        size_t colorMemory = rt->GetColorAttachmentCount() * rt->GetWidth() * rt->GetHeight() * 4;
        
        // Depth attachment (approximate 4 bytes per pixel)
        size_t depthMemory = rt->GetDepthTexture() ? rt->GetWidth() * rt->GetHeight() * 4 : 0;
        
        totalBytes += colorMemory + depthMemory;
    }
    
    // Estimate memory for MSAA targets (multiply by sample count)
    for (const auto& pair : m_msaaTargets)
    {
        auto rt = pair.second;
        if (!rt->IsValid()) continue;
        size_t memory = rt->GetWidth() * rt->GetHeight() * 4 * rt->GetSamples();
        totalBytes += memory;
    }
    
    // Estimate memory for cubemap targets (6 faces)
    for (const auto& pair : m_cubemapTargets)
    {
        auto rt = pair.second;
        if (!rt->IsValid()) continue;
        size_t memory = rt->GetSize() * rt->GetSize() * 6 * 4; // 6 faces, RGBA
        totalBytes += memory;
    }
    
    // Estimate memory for array targets (multiply by layer count)
    for (const auto& pair : m_arrayTargets)
    {
        auto rt = pair.second;
        if (!rt->IsValid()) continue;
        size_t memory = rt->GetWidth() * rt->GetHeight() * rt->GetLayerCount() * 4;
        totalBytes += memory;
    }
    
    return totalBytes;
}

void RenderTargetManager::PrintStats() const
{
    LogInfo("========================================");
    LogInfo("  Render Target Manager Statistics");
    LogInfo("========================================");
    LogInfo("  Total Render Targets: %zu", GetCount());
    LogInfo("  - Normal: %zu", m_targets.size());
    LogInfo("  - MSAA: %zu", m_msaaTargets.size());
    LogInfo("  - Cubemap: %zu", m_cubemapTargets.size());
    LogInfo("  - Array: %zu", m_arrayTargets.size());
    LogInfo("  Estimated Memory Usage: %.2f MB", GetMemoryUsage() / (1024.0f * 1024.0f));
    LogInfo("========================================");
}

void RenderTargetManager::ListAll() const
{
    LogInfo("========================================");
    LogInfo("  All Render Targets");
    LogInfo("========================================");
    
    if (!m_targets.empty())
    {
        LogInfo("--- Normal Render Targets ---");
        for (const auto& pair : m_targets)
        {
            auto rt = pair.second;
            LogInfo("  - '%s': %ux%u, %zu attachments, depth: %s",
                   pair.first.c_str(),
                   rt->GetWidth(), rt->GetHeight(),
                   rt->GetColorAttachmentCount(),
                   rt->GetDepthTexture() ? "Yes" : "No");
        }
    }
    
    if (!m_msaaTargets.empty())
    {
        LogInfo("--- MSAA Render Targets ---");
        for (const auto& pair : m_msaaTargets)
        {
            auto rt = pair.second;
            LogInfo("  - '%s': %ux%u, %ux samples",
                   pair.first.c_str(),
                   rt->GetWidth(), rt->GetHeight(),
                   rt->GetSamples());
        }
    }
    
    if (!m_cubemapTargets.empty())
    {
        LogInfo("--- Cubemap Render Targets ---");
        for (const auto& pair : m_cubemapTargets)
        {
            auto rt = pair.second;
            LogInfo("  - '%s': %ux%u (cubemap)",
                   pair.first.c_str(),
                   rt->GetSize(), rt->GetSize());
        }
    }
    
    if (!m_arrayTargets.empty())
    {
        LogInfo("--- Array Render Targets ---");
        for (const auto& pair : m_arrayTargets)
        {
            auto rt = pair.second;
            LogInfo("  - '%s': %ux%u, %u layers",
                   pair.first.c_str(),
                   rt->GetWidth(), rt->GetHeight(),
                   rt->GetLayerCount());
        }
    }
    
    if (GetCount() == 0)
    {
        LogInfo("  (No render targets created)");
    }
    
    LogInfo("========================================");
}

bool RenderTargetManager::Exists(const std::string& name) const
{
    return m_targets.find(name) != m_targets.end() ||
           m_msaaTargets.find(name) != m_msaaTargets.end() ||
           m_cubemapTargets.find(name) != m_cubemapTargets.end() ||
           m_arrayTargets.find(name) != m_arrayTargets.end();
}