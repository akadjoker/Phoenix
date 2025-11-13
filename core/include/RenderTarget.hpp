#pragma once
#include "Config.hpp"
#include "GraphicsTypes.hpp" 
 
#include <vector>
#include <unordered_map>
#include <string>

// ============================================
// FORWARD DECLARATIONS
// ============================================

class RenderTarget;
class MSAARenderTarget;
class CubemapRenderTarget;
class ArrayRenderTarget;
class Texture;

// ============================================
// ATTACHMENT CONFIGURATION
// ============================================

struct AttachmentConfig
{
    AttachmentType type = AttachmentType::COLOR;
    TextureFormat format = TextureFormat::RGBA8;
    FilterMode minFilter = FilterMode::LINEAR;
    FilterMode magFilter = FilterMode::LINEAR;
    WrapMode wrap = WrapMode::CLAMP_TO_EDGE;
    u32 colorAttachmentIndex = 0;  // For MRT (0-3 guaranteed in GLES 3.0+)
    bool useRenderbuffer = false;  // If true, uses RBO instead of texture (cannot be sampled)
    u32 samples = 1;               // MSAA samples (1, 2, 4, 8... device dependent)
};

// ============================================
// BASE RENDER TARGET
// ============================================

class RenderTarget
{
public:
    RenderTarget();
    virtual ~RenderTarget();

    // Non-copyable, movable
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&& other) noexcept;
    RenderTarget& operator=(RenderTarget&& other) noexcept;

    // === CREATION ===
    bool Create(u32 width, u32 height);
    
    // === ATTACHMENT MANAGEMENT ===
    u32 AddColorAttachment(TextureFormat format = TextureFormat::RGBA8);
    u32 AddColorAttachment(const AttachmentConfig& config);
    bool AddDepthAttachment(TextureFormat format = TextureFormat::DEPTH24);
    bool AddDepthAttachment(const AttachmentConfig& config);
    bool AddDepthStencilAttachment();


    bool AddDepthTexture(TextureFormat format = TextureFormat::DEPTH24); 
    bool AddDepth24Texture() { return AddDepthTexture(TextureFormat::DEPTH24); }
    bool AddDepth32FTexture() { return AddDepthTexture(TextureFormat::DEPTH32F); }
    
    // === FINALIZATION (must be called after adding attachments) ===
    bool Finalize();
    
    // === USAGE ===
    void Bind() const;
    void Unbind() const;
    void Clear(bool clearColor = true, bool clearDepth = true, bool clearStencil = false);
    void ClearColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void ClearColorAttachment(u32 index, float r, float g, float b, float a);
    
    // === RESIZE ===
    bool Resize(u32 newWidth, u32 newHeight);
    
    // === BLIT OPERATIONS ===
    void BlitTo(RenderTarget* target, bool blitColor = true, bool blitDepth = false);
    void BlitToScreen(u32 screenWidth, u32 screenHeight, bool blitColor = true, bool blitDepth = false);
    
    // === GETTERS ===
    u32 GetFBO() const { return m_fbo; }
    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    bool IsValid() const { return m_isValid; }
    bool IsFinalized() const { return m_isFinalized; }
    
    Texture* GetColorTexture(u32 index = 0);
    Texture* GetDepthTexture();
    u32 GetColorTextureID(u32 index = 0) const;
    u32 GetDepthTextureID() const;
    size_t GetColorAttachmentCount() const { return m_colorAttachments.size(); }
    
    // === DEBUG ===
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }
    bool CheckStatus() const;
    void PrintInfo() const;
    
    // === CLEANUP ===
    virtual void Release();

protected:
    struct ColorAttachment
    {
        Texture* texture = nullptr;
        u32 rbo = 0;
        AttachmentConfig config;
    };
    
    struct DepthAttachment
    {
        Texture* texture = nullptr;
        u32 rbo = 0;
        AttachmentConfig config;
        bool hasStencil = false;
    };
    
    void CreateColorAttachment(ColorAttachment& att, u32 attachmentIndex);
    void CreateDepthAttachment(DepthAttachment& att);
    void UpdateDrawBuffers();
    
    u32 m_fbo = 0;
    u32 m_width = 0;
    u32 m_height = 0;
    bool m_isValid = false;
    bool m_isFinalized = false;
  
    
    std::vector<ColorAttachment> m_colorAttachments;
    DepthAttachment m_depthAttachment;
    bool m_hasDepth = false;
    
    std::string m_name = "Unnamed";
};

// ============================================
// MSAA RENDER TARGET
// ============================================

class MSAARenderTarget
{
public:
    MSAARenderTarget();
    ~MSAARenderTarget();

    // Non-copyable, movable
    MSAARenderTarget(const MSAARenderTarget&) = delete;
    MSAARenderTarget& operator=(const MSAARenderTarget&) = delete;
    MSAARenderTarget(MSAARenderTarget&& other) noexcept;
    MSAARenderTarget& operator=(MSAARenderTarget&& other) noexcept;

    // Create MSAA framebuffer with specified sample count
    // Samples will be clamped to device maximum
    bool Create(u32 width, u32 height, u32 samples = 4);
    
    // Add attachments (uses renderbuffers with MSAA)
    u32 AddColorAttachment(TextureFormat format = TextureFormat::RGBA8);
    bool AddDepthAttachment(TextureFormat format = TextureFormat::DEPTH24);
    bool Finalize();
    
    // Bind/Clear
    void Bind() const;
    void Unbind() const;
    void Clear(bool clearColor = true, bool clearDepth = true);
    
    // Resolve MSAA to normal render target
    void ResolveTo(RenderTarget* target, bool color = true, bool depth = false);
    
    // Getters
    u32 GetFBO() const { return m_fbo; }
    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    u32 GetSamples() const { return m_samples; }
    bool IsValid() const { return m_isValid; }
    
    void SetName(const std::string& name) { m_name = name; }
    void Release();

private:
    u32 m_fbo = 0;
    u32 m_width = 0, m_height = 0;
    u32 m_samples = 4;
    std::vector<u32> m_colorRBOs;
    u32 m_depthRBO = 0;
    bool m_isValid = false;
    bool m_isFinalized = false;
    std::string m_name = "MSAA";
};

// ============================================
// CUBEMAP RENDER TARGET (for environment maps)
// ============================================

class CubemapRenderTarget
{
public:
    CubemapRenderTarget();
    ~CubemapRenderTarget();

    // Non-copyable, movable
    CubemapRenderTarget(const CubemapRenderTarget&) = delete;
    CubemapRenderTarget& operator=(const CubemapRenderTarget&) = delete;
    CubemapRenderTarget(CubemapRenderTarget&& other) noexcept;
    CubemapRenderTarget& operator=(CubemapRenderTarget&& other) noexcept;

    // Create cubemap render target
    bool Create(u32 size, TextureFormat colorFormat = TextureFormat::RGBA8, bool withDepth = true);
    
    // Bind specific face for rendering (0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z)
    void BindFace(u32 face);
    void Unbind() const;
    void Clear(bool clearColor = true, bool clearDepth = true);
    
    // Get resulting cubemap texture
    Texture* GetCubemapTexture() { return m_cubemap; }
    u32 GetCubemapTextureID() const;
    u32 GetSize() const { return m_size; }
    bool IsValid() const { return m_isValid; }
    
    void SetName(const std::string& name) { m_name = name; }
    void Release();

private:
    u32 m_fbo = 0;
    u32 m_size = 0;
    Texture* m_cubemap = nullptr;
    u32 m_depthRBO = 0;
    bool m_isValid = false;
    std::string m_name = "Cubemap";
};

// ============================================
// ARRAY RENDER TARGET (for cascaded shadows)
// ============================================

class ArrayRenderTarget
{
public:
    ArrayRenderTarget();
    ~ArrayRenderTarget();

    // Non-copyable, movable
    ArrayRenderTarget(const ArrayRenderTarget&) = delete;
    ArrayRenderTarget& operator=(const ArrayRenderTarget&) = delete;
    ArrayRenderTarget(ArrayRenderTarget&& other) noexcept;
    ArrayRenderTarget& operator=(ArrayRenderTarget&& other) noexcept;

    // Create depth array (useful for cascaded shadow maps)
    bool CreateDepthArray(u32 width, u32 height, u32 layers);
    
    // Create color array
    bool CreateColorArray(u32 width, u32 height, u32 layers, TextureFormat format = TextureFormat::RGBA8);
    
    // Bind specific layer for rendering
    void BindLayer(u32 layer);
    void Unbind() const;
    void Clear(bool clearColor = true, bool clearDepth = true);
    
    // Get resulting array texture
    Texture* GetArrayTexture() { return m_arrayTexture; }
    u32 GetLayerCount() const { return m_layers; }
    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    bool IsValid() const { return m_isValid; }
    
    void SetName(const std::string& name) { m_name = name; }
    void Release();

private:
    u32 m_fbo = 0;
    u32 m_width = 0, m_height = 0;
    u32 m_layers = 0;
    Texture* m_arrayTexture = nullptr;
    bool m_isDepthOnly = false;
    bool m_isValid = false;
    std::string m_name = "Array";
};

// ============================================
// BUILDER PATTERN (fluent interface)
// ============================================

class RenderTargetBuilder
{
public:
    RenderTargetBuilder(u32 width, u32 height);
    
    // Color attachment builders
    RenderTargetBuilder& AddColorRGBA8();
    RenderTargetBuilder& AddColorRGBA16F();
    RenderTargetBuilder& AddColorRGBA32F();
    RenderTargetBuilder& AddColorR8();
    RenderTargetBuilder& AddColorR16F();
    RenderTargetBuilder& AddColorR32F();
    RenderTargetBuilder& AddColor(TextureFormat format, FilterMode filter = FilterMode::LINEAR);
    RenderTargetBuilder& AddColor(const AttachmentConfig& config);
    
    // Depth attachment builders
    RenderTargetBuilder& WithDepth(TextureFormat format = TextureFormat::DEPTH24);
    RenderTargetBuilder& WithDepthStencil();
    
    // Configuration
    RenderTargetBuilder& SetName(const std::string& name);
    
    // Build final render target
    RenderTarget* Build();
    
private:
    u32 m_width, m_height;
    std::string m_name;
    std::vector<AttachmentConfig> m_colorConfigs;
    AttachmentConfig m_depthConfig;
    bool m_hasDepth = false;
    bool m_hasDepthStencil = false;
};

// ============================================
// RENDER TARGET MANAGER (Singleton)
// ============================================

class RenderTargetManager
{
public:
    // Singleton instance
    static RenderTargetManager& Instance();
    
    // === GENERIC CREATION ===
    RenderTarget* Create(const std::string& name, u32 width, u32 height);
    MSAARenderTarget* CreateMSAA(const std::string& name, u32 width, u32 height, u32 samples = 4);
    CubemapRenderTarget* CreateCubemap(const std::string& name, u32 size);
    ArrayRenderTarget* CreateArray(const std::string& name, u32 width, u32 height, u32 layers);
    
    // === FACTORY METHODS (common use cases) ===
    
    // Shadow map (depth only, single layer)
    RenderTarget* CreateShadowMap(const std::string& name, u32 resolution = 2048);
    
    // Cascaded shadow maps (depth array, multiple layers)
    ArrayRenderTarget* CreateCascadedShadowMap(const std::string& name, u32 resolution = 2048, u32 cascades = 4);
    
    // HDR render target (RGBA16F + depth)
    RenderTarget* CreateHDR(const std::string& name, u32 width, u32 height);
    
    // G-Buffer for deferred rendering (Position, Normal, Albedo+Spec + depth)
    RenderTarget* CreateGBuffer(const std::string& name, u32 width, u32 height);
    
    // SSAO buffer (R32F single channel)
    RenderTarget* CreateSSAO(const std::string& name, u32 width, u32 height);
    
    // Bloom/blur buffer (RGBA16F, half resolution typical)
    RenderTarget* CreateBloom(const std::string& name, u32 width, u32 height);
    
    // Water reflection/refraction
    RenderTarget* CreateWaterReflection(const std::string& name, u32 resolution = 512);
    RenderTarget* CreateWaterRefraction(const std::string& name, u32 resolution = 1024);
    
    // Environment cubemap for reflections
    CubemapRenderTarget* CreateEnvironmentMap(const std::string& name, u32 size = 512);
    
    // === RETRIEVAL ===
    RenderTarget* Get(const std::string& name);
    MSAARenderTarget* GetMSAA(const std::string& name);
    CubemapRenderTarget* GetCubemap(const std::string& name);
    ArrayRenderTarget* GetArray(const std::string& name);
    
    // === REMOVAL ===
    void Remove(const std::string& name);
    void UnloadAll();
    
    // === STATISTICS ===
    size_t GetCount() const;
    size_t GetMemoryUsage() const; // Estimate in bytes
    void PrintStats() const;
    void ListAll() const;
    
    // === UTILITY ===
    bool Exists(const std::string& name) const;
    
    // === DEVICE LIMITS (GLES 3.2 specific) ===
    static u32 GetMaxColorAttachments();
    static u32 GetMaxSamples();
    static u32 GetMaxArrayLayers();
    static u32 GetMaxCubemapSize();
    static u32 GetMaxRenderTargetSize();
    
private:
    RenderTargetManager() = default;
    ~RenderTargetManager();
    
    // Disable copy
    RenderTargetManager(const RenderTargetManager&) = delete;
    RenderTargetManager& operator=(const RenderTargetManager&) = delete;
    
    // Storage for different target types
    std::unordered_map<std::string, RenderTarget*> m_targets;
    std::unordered_map<std::string, MSAARenderTarget*> m_msaaTargets;
    std::unordered_map<std::string, CubemapRenderTarget*> m_cubemapTargets;
    std::unordered_map<std::string, ArrayRenderTarget*> m_arrayTargets;
};