

#include "Core.hpp"
#include <vector>
#include <random>

int screenWidth = 1024;
int screenHeight = 768;

class QuadRenderer
{
private:
    GLuint quadVAO, quadVBO;

    void setupQuad()
    {
        float quadVertices[] = {
            // positions        // texture coords
            -1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f, // Top-left
            -1.0f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f, // Bottom-left
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f, // Top-right
            1.0f,
            -1.0f,
            0.0f,
            1.0f,
            0.0f, // Bottom-right
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);

        // Position attribute (3 floats)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

        // Texture coord attribute (2 floats)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

public:
    QuadRenderer()
    {
        setupQuad();
    }
    void render(float x, float y, float w, float h, int screenWidth, int screenHeight)
    {
        float left = (2.0f * x) / screenWidth - 1.0f;
        float right = (2.0f * (x + w)) / screenWidth - 1.0f;
        float top = 1.0f - (2.0f * y) / screenHeight;
        float bottom = 1.0f - (2.0f * (y + h)) / screenHeight;

        float quadVertices[] = {
            // positions           // texture coords
            left,
            top,
            0.0f,
            0.0f,
            1.0f, // Top-left
            left,
            bottom,
            0.0f,
            0.0f,
            0.0f, // Bottom-left
            right,
            top,
            0.0f,
            1.0f,
            1.0f, // Top-right
            right,
            bottom,
            0.0f,
            1.0f,
            0.0f, // Bottom-right
        };

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void render()
    {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
};

// ===== GEOMETRY PASS SHADERS =====
const char *geometryVertexShader = R"(
#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vec4 viewPos = uView * uModel * vec4(aPosition, 1.0);
    FragPos = viewPos.xyz;
    Normal = mat3(transpose(inverse(uView * uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uProjection * viewPos;
}
)";

const char *geometryFragmentShader = R"(
#version 300 es
precision highp float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedo;


uniform bool useTexture;
uniform sampler2D diffuseTexture;
uniform vec3 materialColor;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    vec3 albedo = materialColor;
    if (useTexture) {
        albedo = texture(diffuseTexture, TexCoord).rgb;
    }
    
    gAlbedo = vec4(albedo, 1.0);
}
)";

// ===== SSAO SHADERS =====
const char *ssaoVertexShader = R"(
#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPosition,  1.0);
}
)";

const char *ssaoFragmentShader = R"(
#version 300 es
precision highp float;

in vec2 TexCoord;
out float FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform vec2 noiseScale;

const int kernelSize = 64;
uniform float radius;
uniform float bias;

 

void main() {
    vec3 fragPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);
    
    // Criar TBN (tangent-bitangent-normal)
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i) {
        // Sample position em view-space
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;
        
        // Projetar sample position
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        // Obter profundidade do sample
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = occlusion;
}
)";

// ===== BLUR SHADER (para suavizar SSAO) =====
const char *blurFragmentShader = R"(
#version 300 es
precision highp float;

in vec2 TexCoord;
out float FragColor;

uniform sampler2D ssaoInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {  
         for (int y = -2; y < 2; ++y) 
        {  
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, TexCoord + offset).r;
        }
    }
    FragColor = result / (4.0 * 4.0);
}
)";

// ===== LIGHTING PASS SHADER =====
const char *lightingFragmentShader = R"(
#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

struct Light {
    vec3 Position;
    vec3 Color;
    float Linear;
    float Quadratic;
};

uniform Light light;

uniform float ssaoPower;
uniform float ambientStrength;

void main() {
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
    vec3 Normal = texture(gNormal, TexCoord).rgb;
    vec3 Albedo = texture(gAlbedo, TexCoord).rgb;
    float AmbientOcclusion = texture(ssao, TexCoord).r;

    AmbientOcclusion = pow(AmbientOcclusion, ssaoPower); // 1.5, 2.0, 2.5, 3.0
  
    
    // Ambient
    vec3 ambient = vec3(ambientStrength * Albedo * AmbientOcclusion);
    
    // Diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * light.Color;
    
    // Attenuation
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    
    diffuse *= attenuation;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}
)";

const char *debugVertexShader = R"(
#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}

)";

const char *debugFagmentShader = R"(
#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;

 

uniform sampler2D debugTexture;
uniform int debugMode; // 0=direct, 1=normal, 2=depth, 3=ao

void main() {
    if (debugMode == 0) {
        // Direct RGB
        FragColor = vec4(texture(debugTexture, TexCoord).rgb, 1.0);
    } else if (debugMode == 1) {
        // Normals (remap from [-1,1] to [0,1])
        vec3 normal = texture(debugTexture, TexCoord).rgb;
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
    } else if (debugMode == 2) {
        // Depth visualization
        float depth = texture(debugTexture, TexCoord).r;
        FragColor = vec4(vec3(depth), 1.0);
    } else if (debugMode == 3) {
        // AO (single channel)
        float ao = texture(debugTexture, TexCoord).r;
        FragColor = vec4(vec3(ao), 1.0);
    }
}
)";

// ===== CLASSE SSAO =====
class SSAORenderer
{
public:
    GLuint gBuffer, gPosition, gNormal, gAlbedo;
    GLuint ssaoFBO, ssaoColorBuffer, ssaoBlurFBO, ssaoColorBufferBlur;
    GLuint noiseTexture;
    QuadRenderer quad;
    Shader *gemetry;
    Shader *ssao;
    Shader *blur;
    Shader *lighting;
    Shader *debugTexture;
    bool showDebug = false;
    int debugMode = 0;
    float ssaoPower = 3.0f;       // Contraste do SSAO
    float ssaoRadius = 0.8f;      // Raio de amostragem
    float ssaoBias = 0.015f;      // Bias para evitar self-shadowing
    float ambientStrength = 0.4f; // Força do ambient light
    int ssaoKernelSize = 64;      // Número de samples (fixo, mas podes mostrar)

private:
    int screenWidth, screenHeight;

    float lerp(float a, float b, float f)
    {
        return a + f * (b - a);
    }

public:
    std::vector<Vec3> ssaoKernel;
    std::vector<Vec3> ssaoNoise;
    SSAORenderer(int width, int height) : screenWidth(width), screenHeight(height)
    {
        LogInfo("Initializing SSAO Renderer");
        setupGBuffer();
        LogInfo("Initializing SSAO");
        setupSSAO();
        LogInfo("Compiling Shaders");
        generateSSAOKernel();
        LogInfo("Generating Noise Texture");
        generateNoiseTexture();
        LogInfo("Compiling Shaders");
        compileShaders();
    }

    void setupGBuffer()
    {
        glGenFramebuffers(1, &gBuffer);
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, gBuffer));

        // Position texture (RGBA16F)
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

        // Normal texture (RGBA16F)
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

        // Albedo + Specular texture (RGBA8)
        glGenTextures(1, &gAlbedo);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0));

        // Specify which color attachments to use
        GLenum attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, attachments);

        // Depth renderbuffer
        GLuint rboDepth;
        CHECK_GL_ERROR(glGenRenderbuffers(1, &rboDepth));
        CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, rboDepth));
        CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight));
        CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth));

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            LogError("GBuffer framebuffer incomplete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        LogInfo("GBuffer created: %dx%d", screenWidth, screenHeight);
    }

    void setupSSAO()
    {
        // ==================== SSAO FBO ====================
        CHECK_GL_ERROR(glGenFramebuffers(1, &ssaoFBO));
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO));

        // SSAO color buffer (RED channel, float)
        glGenTextures(1, &ssaoColorBuffer);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            LogError("SSAO Framebuffer not complete!");
        }

        // ==================== SSAO BLUR FBO ====================

        CHECK_GL_ERROR(glGenFramebuffers(1, &ssaoBlurFBO));
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO));

        // SSAO blur color buffer
        glGenTextures(1, &ssaoColorBufferBlur);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            LogError("SSAO Blur Framebuffer not complete!");
        }

        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        LogInfo("SSAO framebuffers created: %dx%d", screenWidth, screenHeight);
    }

    void generateSSAOKernel()
    {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
        std::default_random_engine generator;

        for (int i = 0; i < 64; ++i)
        {
            Vec3 sample(
                randomFloats(generator) * 2.0 - 1.0, // [-1, 1]
                randomFloats(generator) * 2.0 - 1.0, // [-1, 1]
                randomFloats(generator)              // [0, 1] hemisphere
            );

            sample = sample.normalized();

            float scale = float(i) / 64.0f;
            scale = lerp(0.1f, 1.0f, scale * scale);
            sample = sample * scale; // ← Multiplica o vetor completo

            ssaoKernel.push_back(sample);
        }
    }

    void generateNoiseTexture()
    {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
        std::default_random_engine generator;

        for (int i = 0; i < 16; i++)
        {
            Vec3 noise(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                0.0f);
            ssaoNoise.push_back(noise);
        }

        glGenTextures(1, &noiseTexture);
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, noiseTexture));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void compileShaders()
    {

        gemetry = ShaderManager::Instance().Create("geometry", geometryVertexShader, geometryFragmentShader);
        gemetry->Bind();
        gemetry->SetUniform("gPosition", 0);
        gemetry->SetUniform("gNormal", 1);
        gemetry->SetUniform("gAlbedo", 2);
        gemetry->SetUniform("useTexture", 0);
        gemetry->SetUniform("materialColor", 1.0f, 1.0f, 1.0f);

        ssao = ShaderManager::Instance().Create("ssao", ssaoVertexShader, ssaoFragmentShader);
        ssao->Bind();
        ssao->SetUniform("gPosition", 0);
        ssao->SetUniform("gNormal", 1);
        ssao->SetUniform("texNoise", 2);
        ssao->SetUniform("radius", ssaoRadius);
        ssao->SetUniform("bias", ssaoBias);

        // noiseScale = (screenWidth/4, screenHeight/4) para 4x4 noise tile
        ssao->SetUniform("noiseScale", float(screenWidth) / 4.0f, float(screenHeight) / 4.0f);
        // envia kernel
        for (int i = 0; i < 64; ++i)
        {
            std::string name = "samples[" + std::to_string(i) + "]";
            ssao->SetUniform(name.c_str(), ssaoKernel[i].x, ssaoKernel[i].y, ssaoKernel[i].z);
        }

        blur = ShaderManager::Instance().Create("blur", ssaoVertexShader, blurFragmentShader);
        blur->Bind();
        blur->SetUniform("ssaoInput", 0);

        lighting = ShaderManager::Instance().Create("lighting", ssaoVertexShader, lightingFragmentShader);
        lighting->Bind();
        lighting->SetUniform("gPosition", 0);
        lighting->SetUniform("gNormal", 1);
        lighting->SetUniform("gAlbedo", 2);
        lighting->SetUniform("ssao", 3);
        lighting->SetUniform("ssaoPower", ssaoPower);
        lighting->SetUniform("ambientStrength", ambientStrength);

        debugTexture = ShaderManager::Instance().Create("debugTexture", debugVertexShader, debugFagmentShader);
        debugTexture->Bind();
        debugTexture->SetUniform("debugMode", 0);
    }

    void render()
    {
        // 1. Geometry Pass
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, gBuffer));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Renderizar geometria...

        // 2. SSAO Pass
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO));
        glClear(GL_COLOR_BUFFER_BIT);
        quad.render();

        // 3. Blur Pass
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO));
        glClear(GL_COLOR_BUFFER_BIT);
        quad.render();

        // 4. Lighting Pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quad.render();
    }
};

int main()
{

    Device &device = Device::Instance();

    if (!device.Create(screenWidth, screenHeight, "Game", true, 1))
    {
        return 1;
    }
    Driver &driver = Driver::Instance();
    driver.SetClearDepth(1.0f);
    driver.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    RenderBatch batch;
    batch.Init();

    Font font;
    font.SetBatch(&batch);
    font.LoadDefaultFont();

    float lastX{0};
    float lastY{0};

    CameraFree camera(45.0f, (float)screenWidth / (float)screenHeight, 1.25f, 1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);

    bool firstMouse{true};
    float mouseSensitivity{0.8f};

    Texture *texture = TextureManager::Instance().Load("wall.jpg", true);
    // texture.SetGenerateMipmaps(true);

    texture->SetAnisotropy(8.0f);
    texture->SetMinFilter(FilterMode::LINEAR_MIPMAP);
    texture->SetMagFilter(FilterMode::LINEAR);
    texture->SetWrap(WrapMode::REPEAT);
    

    Vec3 cubePositions[] = {
        Vec3(0.0f, 0.5f, 0.0f),
        Vec3(2.0f, 0.5f, -5.0f),
        Vec3(-1.5f, 0.5f, -2.5f),
        Vec3(-3.8f, 0.5f, -12.3f),
        Vec3(2.4f, 0.5f, -3.5f),
        Vec3(-1.7f, 0.5f, -7.5f),
        Vec3(1.3f, 0.5f, -2.5f),
        Vec3(1.5f, 0.5f, -10.5f),
        Vec3(5.5f, 0.5f, -15.5f),
        Vec3(-5.3f, 0.5f, -20.5f)};
 

    //  Mesh *plane = MeshManager::Instance().CreateCube("palne", 100);
    
    SSAORenderer ssao(screenWidth, screenHeight);
    Mesh *plane = MeshManager::Instance().CreatePlane("plane", 10, 10, 50, 50);
    Mesh *cube = MeshManager::Instance().CreateCube("cube");

    //Mesh *room = MeshManager::Instance().Import("room", "assets/room.3ds");
   
     Mesh *room = MeshManager::Instance().Load("room", "assets/asponza.h3d");
   //  room->CalculateNormals();
    // Mesh *room = MeshManager::Instance().Import("room", "assets/sponza.obj");
    // room->OptimizeBuffers();
    for (size_t i = 0; i < room->GetBufferCount(); i++)
    {
        room->GetBuffer(i)->Scale(0.08f);
    }
    //MeshManager::Instance().Save("assets/sponza.h3d", room);


    while (device.IsRunning())
    {

        driver.Reset();
        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;

        SDL_Event event;
        while (device.PollEvents(&event))
        {
            if (event.type == SDL_QUIT)
            {
                device.SetShouldClose(true);
                break;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    device.SetShouldClose(true);
                else if (event.key.keysym.sym == SDLK_n)
                    ssao.showDebug = !ssao.showDebug;
                else if (event.key.keysym.sym == SDLK_m)
                {
                    ssao.debugMode = (ssao.debugMode + 1) % 5;
                    LogInfo("Debug mode: %d", ssao.debugMode);
                }
                break;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    screenWidth = event.window.data1;
                    screenHeight = event.window.data2;

                    driver.SetViewPort(0, 0, screenWidth, screenHeight);
                    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
                    break;
                }
                break;
            }
        }

        int xposIn, yposIn;
        u32 IsMouseDown = SDL_GetMouseState(&xposIn, &yposIn);

        if (IsMouseDown & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            float xpos = static_cast<float>(xposIn);
            float ypos = static_cast<float>(yposIn);

            if (firstMouse)
            {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }

            float xoffset = xpos - lastX;
            float yoffset = ypos - lastY;

            lastX = xpos;
            lastY = ypos;
            // fpsCamera.MouseLook(xoffset, yoffset);

            camera.rotate(yoffset * mouseSensitivity, xoffset * mouseSensitivity);
        }
        else
        {
            firstMouse = true;
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W])
            camera.move(SPEED);
        if (state[SDL_SCANCODE_S])
            camera.move(-SPEED);
        if (state[SDL_SCANCODE_A])
            camera.strafe(-SPEED);
        if (state[SDL_SCANCODE_D])
            camera.strafe(SPEED);

        if (state[SDL_SCANCODE_KP_PLUS])
            ssao.ssaoPower += 0.1f * dt * 10.0f;
        if (state[SDL_SCANCODE_KP_MINUS])
            ssao.ssaoPower -= 0.1f * dt * 10.0f;

        if (state[SDL_SCANCODE_KP_8])
            ssao.ssaoRadius += 0.05f * dt * 10.0f;
        if (state[SDL_SCANCODE_KP_2])
            ssao.ssaoRadius -= 0.05f * dt * 10.0f;

        if (state[SDL_SCANCODE_KP_6])
            ssao.ssaoBias += 0.001f * dt * 10.0f;
        if (state[SDL_SCANCODE_KP_4])
            ssao.ssaoBias -= 0.001f * dt * 10.0f;

        if (state[SDL_SCANCODE_KP_9])
            ssao.ambientStrength += 0.05f * dt * 10.0f;
        if (state[SDL_SCANCODE_KP_3])
            ssao.ambientStrength -= 0.05f * dt * 10.0f;

        ssao.ssaoPower = Max(0.1f, Min(ssao.ssaoPower, 5.0f));
        ssao.ssaoRadius = Max(0.1f, Min(ssao.ssaoRadius, 2.0f));
        ssao.ssaoBias = Max(0.001f, Min(ssao.ssaoBias, 0.1f));
        ssao.ambientStrength = Max(0.0f, Min(ssao.ambientStrength, 1.0f));

        camera.update(1.0f);
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();
        const Mat4 &mvp = proj * view;

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.SetCulling(CullMode::Back);

        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        // === 1. GEOMETRY PASS ===
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, ssao.gBuffer));
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        ssao.gemetry->Bind();
        ssao.gemetry->SetUniformMat4("uView", view.m);
        ssao.gemetry->SetUniformMat4("uProjection", proj.m);
        ssao.gemetry->SetUniform("useTexture", true);
        ssao.gemetry->SetTexture2D("diffuseTexture", texture->GetHandle(), 0);

        // Render plane
        Mat4 model;// = Mat4::Translation(0.0f, -0.5f, 0.0f) * Mat4::Scale(5.0f, 1.0f, 5.0f);
        ssao.gemetry->SetUniformMat4("uModel", model.m);
        driver.DrawMesh(room);
        // Render cubes
        for (int i = 0; i < 10; i++)
        {
            model = Mat4::Translation(cubePositions[i]);
            ssao.gemetry->SetUniformMat4("uModel", model.m);
            driver.DrawMesh(cube);
        }

        // === 2. SSAO PASS ===
        glBindFramebuffer(GL_FRAMEBUFFER, ssao.ssaoFBO);
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        ssao.ssao->Bind();
        ssao.ssao->SetUniform("ssaoRadius", ssao.ssaoRadius);
        ssao.ssao->SetUniform("ssaoBias", ssao.ssaoBias);

        // Bind G-Buffer textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssao.gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ssao.gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ssao.noiseTexture);

        // for (int i = 0; i < 64; ++i)
        // {
        //     std::string name = "samples[" + std::to_string(i) + "]";
        //     ssao.ssao->SetUniform(name.c_str(), ssao.ssaoKernel[i].x, ssao.ssaoKernel[i].y, ssao.ssaoKernel[i].z);
        // }

        // Set projection matrix
        ssao.ssao->SetUniformMat4("projection", proj.m);

        ssao.quad.render();

        // === 3. BLUR PASS ===
        glBindFramebuffer(GL_FRAMEBUFFER, ssao.ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        ssao.blur->Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssao.ssaoColorBuffer);

        ssao.quad.render();

        // === 4. LIGHTING PASS ===
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderiza cena ou debug
        if (!ssao.showDebug)
        {
            // Normal lighting
            ssao.lighting->Bind();
            ssao.lighting->SetUniform("ssaoPower", ssao.ssaoPower);
            ssao.lighting->SetUniform("ambientStrength", ssao.ambientStrength);
            // glActiveTexture(GL_TEXTURE0);
            // glBindTexture(GL_TEXTURE_2D, ssao.gPosition);
            // glActiveTexture(GL_TEXTURE1);
            // glBindTexture(GL_TEXTURE_2D, ssao.gNormal);
            // glActiveTexture(GL_TEXTURE2);
            // glBindTexture(GL_TEXTURE_2D, ssao.gAlbedo);
            // glActiveTexture(GL_TEXTURE3);
            // glBindTexture(GL_TEXTURE_2D, ssao.ssaoColorBufferBlur);

            driver.BindTexture(0, GL_TEXTURE_2D, ssao.gPosition);
            driver.BindTexture(1, GL_TEXTURE_2D, ssao.gNormal);
            driver.BindTexture(2, GL_TEXTURE_2D, ssao.gAlbedo);
            driver.BindTexture(3, GL_TEXTURE_2D, ssao.ssaoColorBufferBlur);

            Vec3 lightPos = Vec3(2.0, 4.0, -2.0);
            Vec4 lightPosView4 = view * Vec4(lightPos, 1.0);
            Vec3 lightPosView = Vec3(lightPosView4.x, lightPosView4.y, lightPosView4.z);

            ssao.lighting->SetUniform("light.Position", lightPosView.x, lightPosView.y, lightPosView.z);
            ssao.lighting->SetUniform("light.Color", 1.0f, 1.0f, 1.0f);
            ssao.lighting->SetUniform("light.Linear", 0.09f);
            ssao.lighting->SetUniform("light.Quadratic", 0.032f);

            ssao.quad.render();
        }
        else
        {
            // Debug fullscreen
            driver.SetDepthTest(false);
            ssao.debugTexture->Bind();

            GLuint debugTex = ssao.gAlbedo;
            int debugModeValue = 0;

            switch (ssao.debugMode)
            {
            case 0:
                debugTex = ssao.gAlbedo;
                debugModeValue = 0;
                break;
            case 1:
                debugTex = ssao.gNormal;
                debugModeValue = 1;
                break;
            case 2:
                debugTex = ssao.gPosition;
                debugModeValue = 2;
                break;
            case 3:
                debugTex = ssao.ssaoColorBuffer;
                debugModeValue = 3;
                break;
            case 4:
                debugTex = ssao.ssaoColorBufferBlur;
                debugModeValue = 3;
                break;
            }

            ssao.debugTexture->SetUniform("debugMode", debugModeValue);
            //glActiveTexture(GL_TEXTURE0);
            //glBindTexture(GL_TEXTURE_2D, debugTex);
            driver.BindTexture(0, GL_TEXTURE_2D, debugTex);
            ssao.quad.render();
        }

        // UI sempre por cima
        batch.SetMatrix(ortho);
        driver.SetBlendEnable(true);
        driver.SetCulling(CullMode::None);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        batch.SetColor(255, 255, 255);
        font.Print(10, 10, "FPS: %d", device.GetFPS());

        font.Print(10, 30, "SSAO Power:     %.2f  [+/-]", ssao.ssaoPower);
        font.Print(10, 50, "SSAO Radius:    %.2f  [8/2]", ssao.ssaoRadius);
        font.Print(10, 70, "SSAO Bias:      %.3f  [6/4]", ssao.ssaoBias);
        font.Print(10, 90, "Ambient:        %.2f  [9/3]", ssao.ambientStrength);

        if (ssao.showDebug)
        {
            const char *modes[] = {"Albedo", "Normals", "Position", "SSAO Raw", "SSAO Blur"};
            font.Print(10, screenHeight - 40, "Debug: %s (M=cycle, N=toggle)", modes[ssao.debugMode]);
        }


        //stas 
        int y = screenHeight - 40;
        font.Print(10,y,"Tris: %d Vertices: %d", driver.GetCountTriangle(), driver.GetCountVertex());
        font.Print(10,y - 20,"Draw Calls: %d Meshes: %d MeshBuffers: %d", driver.GetCountDrawCall(),driver.GetCountMesh(),driver.GetCountMeshBuffer());
        font.Print(10,y - 40,"Textures: %d Programs: %d", driver.GetCountTextures(),driver.GetCountPrograms());


        batch.Render();

        driver.SetBlendEnable(false);

        device.Flip();
    }

    font.Release();
    batch.Release();

    MeshManager::Instance().UnloadAll();
    ShaderManager::Instance().UnloadAll();
    TextureManager::Instance().UnloadAll();
    device.Close();

    return 0;
}
