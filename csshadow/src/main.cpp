

#include "Core.hpp"
#include <vector>
#include <random> 

int screenWidth = 1024;
int screenHeight = 768;

// Configuração CSM
const int CASCADE_COUNT = 4;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

struct CascadeData
{
    float splitDepth;
    Mat4 viewProjMatrix;
};

class QuadRenderer
{
private:
    GLuint quadVAO, quadVBO;

    void setupQuad()
    {
        float quadVertices[] = {
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

public:
    QuadRenderer() { setupQuad(); }
    
    void render(float x, float y, float w, float h, int screenWidth, int screenHeight)
    {
        float left = (2.0f * x) / screenWidth - 1.0f;
        float right = (2.0f * (x + w)) / screenWidth - 1.0f;
        float top = 1.0f - (2.0f * y) / screenHeight;
        float bottom = 1.0f - (2.0f * (y + h)) / screenHeight;

        float quadVertices[] = {
            left, top, 0.0f, 0.0f, 1.0f,
            left, bottom, 0.0f, 0.0f, 0.0f,
            right, top, 0.0f, 1.0f, 1.0f,
            right, bottom, 0.0f, 1.0f, 0.0f,
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


std::vector<Vec4> getFrustumCornersWorldSpace(const Mat4& proj, const Mat4& view)
{
    
    const auto inv = Mat4::Inverse(proj * view);
    std::vector<Vec4> frustumCorners;
    
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const Vec4 pt = inv * Vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}

Mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane,
                               const Mat4& proj, const Mat4& view,
                               const Vec3& lightDir)
{
    const auto corners = getFrustumCornersWorldSpace(proj, view);
    
    Vec3 center = Vec3(0, 0, 0);
    for (const auto& v : corners)
    {
        center.x += v.x;
        center.y += v.y;
        center.z += v.z;
    }
    center /= corners.size();

    const auto lightView = Mat4::LookAt(center + lightDir, center, Vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }
 
    constexpr float zMult = 10.0f;
    if (minZ < 0)
    {
        minZ *= zMult;
    }
    else
    {
        minZ /= zMult;
    }
    if (maxZ < 0)
    {
        maxZ /= zMult;
    }
    else
    {
        maxZ *= zMult;
    }

    const Mat4 lightProjection = Mat4::Ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<float> getCascadeSplits(float nearPlane, float farPlane, int cascadeCount)
{
    std::vector<float> splits;
    splits.resize(cascadeCount);
    
    float lambda = 0.75f;  
    
    for (int i = 0; i < cascadeCount; ++i)
    {
        float p = (i + 1) / static_cast<float>(cascadeCount);
        float log = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniform = nearPlane + (farPlane - nearPlane) * p;
        splits[i] = lambda * log + (1.0f - lambda) * uniform;
    }
    
    return splits;
}

const char *depthVertexShader = R"(
#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char *depthFragmentShader = R"(
#version 300 es
precision highp float;

void main()
{             
}
)";

const char *debugVertexShader = R"(
#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
)";

const char *debugFragmentShader = R"(
#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}
)";

const char *shadowVertexShader = R"(
#version 300 es
precision highp float;
 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosViewSpace;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoords = aTexCoords;
    FragPosViewSpace = view * vec4(FragPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *shadowFragmentShader = R"(
#version 300 es
precision highp float;
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosViewSpace;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap[4];

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrices[4];
uniform float cascadePlaneDistances[4];
uniform int cascadeCount;
uniform bool showCascades;
uniform float farPlane;
uniform vec2 shadowMapSize;

float SampleShadowMap(int cascadeIndex, vec2 uv)
{
    if(cascadeIndex == 0)
        return texture(shadowMap[0], uv).r;
    else if(cascadeIndex == 1)
        return texture(shadowMap[1], uv).r;
    else
        if(cascadeIndex == 2)
            return texture(shadowMap[2], uv).r;
        else if(cascadeIndex == 3)
            return texture(shadowMap[3], uv).r;
    
    return texture(shadowMap[0], uv).r;
}

float ShadowCalculation2(int cascadeIndex, vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float closestDepth = SampleShadowMap(cascadeIndex, projCoords.xy); 
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Bias melhorado: constante + slope, com tan(acos) para precisão em ângulos
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float slopeBias = 0.001 * tan(acos(cosTheta));  // Mantido, mas agora somado a constante
    float constantBias = 0.0005;  // Adicionado: bias fixo para superfícies planas
    float baseBias = constantBias + slopeBias;
    
    // Escala adaptativa por cascata: projeta bias de mundo para espaço de profundidade
    // Usa a distância da cascata para escalar (maior bias em mundo para cascatas distantes)
    float cascadeDistance = (cascadeIndex == cascadeCount - 1) ? farPlane : cascadePlaneDistances[cascadeIndex];
    float depthScale = cascadeDistance / farPlane;  // Normaliza pela far plane total
    const float worldBiasFactor = 0.02;  // Ajuste em unidades de mundo (tune isso!)
    float worldBias = worldBiasFactor * depthScale;  // Bias maior para distâncias maiores
    
    // Projeta para espaço de profundidade (aproximação simples via derivada de profundidade)
    float depthBias = baseBias + (worldBias / (farPlane * 100.0));  // Escala para [0-1]; ajuste o 100.0 pela resolução
    
    // Clamp para evitar exageros (descomentei e ajustei baseado em testes comuns)
    depthBias = clamp(depthBias, 0.0001, 0.005 * depthScale);  // Menor clamp para cascatas próximas, maior para distantes
    
    // Ajuste final por resolução do shadow map (opcional, mas melhora em resoluções baixas)
    vec2 texelSize = 1.0 / shadowMapSize;
    depthBias *= (1.0 + texelSize.x * 2.0);  // Fator pequeno baseado em texel para anti-acne
 
    float shadow = 0.0;
    vec2 texelSizePCF = 1.0 / shadowMapSize;
    int pcfCount = 2;
    int numSamples = (pcfCount * 2 + 1) * (pcfCount * 2 + 1);
    
    for(int x = -pcfCount; x <= pcfCount; ++x)
    {
        for(int y = -pcfCount; y <= pcfCount; ++y)
        {
            float pcfDepth = SampleShadowMap(cascadeIndex, projCoords.xy + vec2(x, y) * texelSizePCF); 
            shadow += currentDepth - depthBias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= float(numSamples);
    
    return shadow;
}

float ShadowCalculation(int cascadeIndex, vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float closestDepth = SampleShadowMap(cascadeIndex, projCoords.xy); 
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Bias melhorado baseado no slope
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    //float bias = max(0.08 * (1.0 - dot(normal, lightDir)), 0.008);  
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float bias = 0.001 * tan(acos(cosTheta));
    
    const float biasModifier = 0.5;
    if (cascadeIndex == cascadeCount - 1)
    {
        bias *= 1.0 / (farPlane * biasModifier);
    } 
    else if (cascadeIndex == 0)
    {
         bias = 0.0009;
    } 
    else 
    {
        bias *= 1.0 / (cascadePlaneDistances[cascadeIndex] * biasModifier);
    }

 
   // bias = clamp(bias, 0.0001, 0.01);
   bias = clamp(bias, 0.00005, 0.01);
    
  
    float shadow = 0.0;
    vec2 texelSize = 1.0 / shadowMapSize;
    int pcfCount = 2;
    int numSamples = (pcfCount * 2 + 1) * (pcfCount * 2 + 1);
    
    for(int x = -pcfCount; x <= pcfCount; ++x)
    {
        for(int y = -pcfCount; y <= pcfCount; ++y)
        {
            float pcfDepth = SampleShadowMap(cascadeIndex, projCoords.xy + vec2(x, y) * texelSize); 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= float(numSamples);
    
    return shadow;
}


vec3 GetCascadeColor(int cascadeIndex)
{
    if(cascadeIndex == 0)
        return vec3(1.0, 0.0, 0.0); // Vermelho
    else if(cascadeIndex == 1)
        return vec3(0.0, 1.0, 0.0); // Verde
    else if(cascadeIndex == 2)
        return vec3(0.0, 0.0, 1.0); // Azul
    else
        if (cascadeIndex == 3)
            return vec3(1.0, 1.0, 0.0); // Amarelo
    return vec3(1.0, 1.0, 1.0);
}

void main()
{           
    vec3 color = texture(diffuseTexture, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(1.0);
    
    // Ambient
    vec3 ambient = 0.15 * lightColor;
    
    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    
    // Selecionar cascata baseado na profundidade
    float depthValue = abs(FragPosViewSpace.z);
    int cascadeIndex = -1;
    
    for(int i = 0; i < cascadeCount - 1; ++i)
    {
        if(depthValue < cascadePlaneDistances[i])
        {
            cascadeIndex = i ;
            break;
        }
    }
    
    if(cascadeIndex == -1)
    {
        cascadeIndex = cascadeCount ;
    }
    
    // Calcular sombra
    vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(cascadeIndex, fragPosLightSpace);
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    
    // Visualização de cascatas (debug)
    if(showCascades)
    {
        lighting *= GetCascadeColor(cascadeIndex);
    }
    
    FragColor = vec4(lighting, 1.0);
}
)";


 
int main()
{

    Device &device = Device::Instance();

    if (!device.Create(screenWidth, screenHeight, "Game", true,1))
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

    CameraFree camera(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);


    bool firstMouse{true};
    float mouseSensitivity{0.8f};



    Texture *texture = TextureManager::Instance().Load("wall.jpg");
    texture->SetAnisotropy(8.0f);
    texture->SetMinFilter(FilterMode::LINEAR_MIPMAP);
    texture->SetMagFilter(FilterMode::LINEAR);

    Mesh *plane = MeshManager::Instance().CreatePlane("plane", 10, 10, 50, 50);
    Mesh *cube = MeshManager::Instance().CreateCube("cube");

    Shader *simpleDepthShader = ShaderManager::Instance().Create("depth", depthVertexShader, depthFragmentShader);
    Shader *debugShader = ShaderManager::Instance().Create("debug", debugVertexShader, debugFragmentShader);
    Shader *shader = ShaderManager::Instance().Create("shader", shadowVertexShader, shadowFragmentShader);

    // Criar framebuffers para cada cascata
    unsigned int depthMapFBO[CASCADE_COUNT];
    unsigned int depthMaps[CASCADE_COUNT];
    unsigned int dummyColors[CASCADE_COUNT];
    
    for (int i = 0; i < CASCADE_COUNT; ++i)
    {
        glGenFramebuffers(1, &depthMapFBO[i]);
        
        glGenTextures(1, &depthMaps[i]);
        glBindTexture(GL_TEXTURE_2D, depthMaps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glGenTextures(1, &dummyColors[i]);
        glBindTexture(GL_TEXTURE_2D, dummyColors[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dummyColors[i], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMaps[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            LogError("ERROR::FRAMEBUFFER:: Cascade %d framebuffer is not complete!", i);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    Vec3 lightPos(-2, 4.0f, -4.0f);

    Vec3 cubePositions[] = {
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(2.0f, 5.0f, -15.0f),
        Vec3(-1.5f, 2.2f, -2.5f),
        Vec3(-3.8f, 2.0f, -12.3f),
        Vec3(2.4f, -0.4f, -3.5f),
        Vec3(-1.7f, 3.0f, -7.5f),
        Vec3(1.3f, 2.0f, -2.5f),
        Vec3(1.5f, 2.0f, -2.5f),
        Vec3(1.5f, 0.2f, -1.5f),
        Vec3(-1.3f, 1.0f, -1.5f)};

    QuadRenderer quad;


    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;

           lightPos.x = sin(device.GetTime()) * 3.0f;
        lightPos.z = cos(device.GetTime()) * 2.0f;
        lightPos.y = 5.0 + cos(device.GetTime()) * 1.0f;

        SDL_Event event;
        while (device.PollEvents(&event))
        {
            if (event.type == SDL_QUIT)
            {
                device.SetShouldClose(true);
                break;
            }else 
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                device.SetShouldClose(true);
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
             //fpsCamera.MouseLook(xoffset, yoffset);

             camera.rotate(yoffset * mouseSensitivity,xoffset * mouseSensitivity);
      
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

        camera.update(1.0f);
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();
        const Mat4 &mvp = proj * view;
        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
        const Vec3 cameraPos = camera.getPosition();


       driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);


       batch.SetMatrix(mvp);
       driver.SetDepthTest(true);
       driver.SetBlendEnable(false);
       batch.Grid(10, 1.0f, true);
       batch.Render();




     float nearPlane = 0.1f;
     float farPlane = 800.0f;
     bool showCascades = false;


        // Calcular splits das cascatas
        auto cascadeSplits = getCascadeSplits(nearPlane, farPlane, CASCADE_COUNT);
        
        std::vector<Mat4> lightSpaceMatrices;
        Vec3 lightDir =lightPos.normalized();
        
        // Gerar matrizes para cada cascata
        float lastSplitDist = nearPlane;
        for (int i = 0; i < CASCADE_COUNT; ++i)
        {
            float splitDist = cascadeSplits[i];
            
            Mat4 proj = Mat4::Perspective(ToRadians(60.0f), 
                (float)screenWidth / (float)screenHeight, 
                lastSplitDist, splitDist);
            
            auto lightMatrix = getLightSpaceMatrix(lastSplitDist, splitDist, proj, view, lightDir);
            lightSpaceMatrices.push_back(lightMatrix);
            
            lastSplitDist = splitDist;
        }

        // Renderizar depth maps para cada cascata
        simpleDepthShader->Bind();
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        Mat4 model;
        
        for (int cascade = 0; cascade < CASCADE_COUNT; ++cascade)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[cascade]);
            glClear(GL_DEPTH_BUFFER_BIT);
            
            simpleDepthShader->SetUniformMat4("lightSpaceMatrix", lightSpaceMatrices[cascade].m);
            
            // Render scene
            
            for (int i = 0; i < 10; i++)
            {
                model = Mat4::Translation(cubePositions[i]);
                simpleDepthShader->SetUniformMat4("model", model.m);
                cube->Render();
            }
        }
       //glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderizar cena com sombras
        shader->Bind();
        shader->SetUniform("diffuseTexture", 0);
        shader->SetUniform("cascadeCount", CASCADE_COUNT);
        shader->SetUniform("showCascades", showCascades ? 1 : 0);
        shader->SetUniformMat4("projection", proj.m);
        shader->SetUniformMat4("view", view.m);
        shader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        shader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        shader->SetUniform("farPlane", farPlane);
        shader->SetUniform("shadowMapSize", SHADOW_WIDTH, SHADOW_HEIGHT);
        
        // Passar matrizes e splits das cascatas
        for (int i = 0; i < CASCADE_COUNT; ++i)
        {
            std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
            shader->SetUniformMat4(uniformName.c_str(), lightSpaceMatrices[i].m);
            
            uniformName = "cascadePlaneDistances[" + std::to_string(i) + "]";
            shader->SetUniform(uniformName.c_str(), cascadeSplits[i]);
        }
        
        shader->SetTexture2D("diffuseTexture", texture->GetHandle(), 0);
        for (int i = 0; i < CASCADE_COUNT; ++i)
        {
            std::string uniformName = "shadowMap[" + std::to_string(i) + "]";
            shader->SetTexture2D(uniformName.c_str(), depthMaps[i], 1 + i);
        }

        // Renderizar plano
         model = Mat4::Scale(Vec3(2.0f));
        shader->SetUniformMat4("model", model.m);
        plane->Render();
        
        // Renderizar cubos
        for (int i = 0; i < 10; i++)
        {
            model = Mat4::Translation(cubePositions[i]) * Mat4::Scale(Vec3(1.0f));
            shader->SetUniformMat4("model", model.m);
            cube->Render();
        }

        // Debug: visualizar cascatas com labels
        debugShader->Bind();
        debugShader->SetUniform("depthMap", 0);
        for (int i = 0; i < CASCADE_COUNT; ++i)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthMaps[i]);
            quad.render(i * 210, 0, 200, 200, screenWidth, screenHeight);
        }
        
        debugShader->Bind();
        debugShader->SetUniform("depthMap", 0);
        for (int i = 0; i < CASCADE_COUNT; ++i)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthMaps[i]);
            quad.render(i * 210, 0, 200, 200, screenWidth, screenHeight);
        }
        


       batch.SetMatrix(ortho);
       driver.SetDepthTest(false);
       driver.SetBlendEnable(true);
       driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

       batch.SetColor(255, 255, 255);
//
       font.Print(10,10,"Fps :%d",device.GetFPS());

       

       batch.Render();
 
 


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
