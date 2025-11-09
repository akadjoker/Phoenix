

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
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

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
    void render()
    {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
    void relase()
    {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
    }
};

const char *bufferVertexShader = R"(
#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;
}

)";

const char *bufferFragmentShader = R"(
#version 300 es
precision highp float;
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
}

)";

const char *deferredVertexShader = R"(
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

const char *deferredFragmentShader = R"(
#version 300 es
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main()
{             
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;
        // attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;        
    }
    FragColor = vec4(lighting, 1.0);
}

)";

const char *solidVertexShader = R"(
#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
 
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
     

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

const char *solidFragmentShader = R"(
#version 300 es
precision highp float;
layout (location = 0) out vec4 FragColor;
uniform vec3 lightColor; 
 

void main()
{    
    FragColor = vec4(lightColor, 1.0);
}

)";

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

    CameraFree camera(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);

    bool firstMouse{true};
    float mouseSensitivity{0.8f};

    Shader *shaderGeometryPass = ShaderManager::Instance().Create("buffer", bufferVertexShader, bufferFragmentShader);
    Shader *shaderLightingPass = ShaderManager::Instance().Create("deferred", deferredVertexShader, deferredFragmentShader);
    Shader *shaderSolid = ShaderManager::Instance().Create("solid", solidVertexShader, solidFragmentShader);

    Mesh *cube = MeshManager::Instance().CreateCube("cube");

    Texture *texture = TextureManager::Instance().Load("assets/wall.jpg");
    Texture *white = TextureManager::Instance().Get("white");

    // configure g-buffer framebuffer
    // ------------------------------
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LogError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    QuadRenderer quad;

    TextureManager::Instance().SetLoadPath("assets/textures/");

    Mesh *meshSponza = MeshManager::Instance().Load("sponza", "assets/sponza.h3d");

    meshSponza->SetTexture(1, white);

    Vec3 cubePositions[] = {
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(2.0f, 5.0f, -15.0f),
        Vec3(-1.5f, -2.2f, -2.5f),
        Vec3(-3.8f, -2.0f, -12.3f),
        Vec3(2.4f, -0.4f, -3.5f),
        Vec3(-1.7f, 3.0f, -7.5f),
        Vec3(1.3f, -2.0f, -2.5f),
        Vec3(1.5f, 2.0f, -2.5f),
        Vec3(1.5f, 0.2f, -1.5f),
        Vec3(-1.3f, 1.0f, -1.5f)};

    const unsigned int NR_LIGHTS = 32;
    std::vector<Vec3> lightPositions;
    std::vector<Vec3> lightColors;

    for (unsigned int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate slightly random offsets
        float xPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
        float yPos = ((rand() % 100) / 100.0f) * 6.0f - 4.0f;
        float zPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
        lightPositions.push_back(Vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = ((rand() % 100) / 200.0f) + 0.5f;
        float gColor = ((rand() % 100) / 200.0f) + 0.5f;
        float bColor = ((rand() % 100) / 200.0f) + 0.5f;
        lightColors.push_back(Vec3(rColor, gColor, bColor));
    }

    shaderLightingPass->Bind();
    shaderLightingPass->SetUniform("gPosition", 0);
    shaderLightingPass->SetUniform("gNormal", 1);
    shaderLightingPass->SetUniform("gAlbedoSpec", 2);

    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;

        //    lightPos.x = sin(device.GetTime()) * 3.0f;
        // lightPos.z = cos(device.GetTime()) * 2.0f;
        // lightPos.y = 25.0 + cos(device.GetTime()) * 1.0f;

        SDL_Event event;
        while (device.PollEvents(&event))
        {
            if (event.type == SDL_QUIT)
            {
                device.SetShouldClose(true);
                break;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
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

        camera.update(1.0f);
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();
        const Mat4 &mvp = proj * view;
        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
        const Vec3 cameraPos = camera.getPosition();

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        static float time = 0.0f;
        time += dt;

        static std::vector<Vec3> initialLightPositions = lightPositions;

        for (unsigned int i = 0; i < NR_LIGHTS; ++i)
        {
            float offset = i * 0.5f;
            float speed = 1.0f + (i % 4) * 0.3f;

            Vec3 basePos = initialLightPositions[i];

            float angle = time * speed + offset;
            float radius = 12.0f;

            lightPositions[i].x = basePos.x + cosf(angle) * radius;
            lightPositions[i].z = basePos.z + sinf(angle) * radius;

            lightPositions[i].y = basePos.y + sinf(time * speed * 1.5f + offset) * 1.5f;
        }
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. geometry pass: render scene's geometry/color data into gbuffer

        shaderGeometryPass->Bind();
        shaderGeometryPass->SetUniformMat4("view", view.m);
        shaderGeometryPass->SetUniformMat4("projection", proj.m);
        shaderGeometryPass->SetUniform("texture_diffuse1", 0);
        shaderGeometryPass->SetUniform("texture_specular1", 1);

        Mat4 model;

        model = Mat4::Scale(0.01f, 0.01f, 0.01f);
        shaderGeometryPass->SetUniformMat4("model", model.m);
        meshSponza->Render();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, white->GetHandle());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->GetHandle());

        for (int i = 0; i < 10; i++)
        {
            model = Mat4::Translation(cubePositions[i]);
            shaderGeometryPass->SetUniformMat4("model", model.m);
            cube->Render();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 

        shaderLightingPass->Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            const Vec3 &pos = lightPositions[i];
            const Vec3 &color = lightColors[i];
            std::string namePosition = "lights[" + std::to_string(i) + "].Position";
            shaderLightingPass->SetUniform(namePosition.c_str(), pos.x, pos.y, pos.z);
            std::string nameColor = "lights[" + std::to_string(i) + "].Color";
            shaderLightingPass->SetUniform(nameColor.c_str(), color.x, color.y, color.z);
            const float linear = 0.7f;
            const float quadratic = 1.8f;
            std::string nameLinear = "lights[" + std::to_string(i) + "].Linear";
            shaderLightingPass->SetUniform(nameLinear.c_str(), linear);
            std::string nameQuadratic = "lights[" + std::to_string(i) + "].Quadratic";
            shaderLightingPass->SetUniform(nameQuadratic.c_str(), quadratic);
        }
        shaderLightingPass->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        quad.render();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);

        // // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        // glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // glEnable(GL_DEPTH_TEST);
        //  glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        shaderSolid->Bind();
        shaderSolid->SetUniformMat4("view", view.m);
        shaderSolid->SetUniformMat4("projection", proj.m);

        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            const Vec3 &pos = lightPositions[i];
            const Vec3 &color = lightColors[i];
            model = Mat4::Translation(pos);
            shaderSolid->SetUniformMat4("model", model.m);
            shaderSolid->SetUniform("lightColor", color.x, color.y, color.z);
            cube->Render();
        }

        batch.SetMatrix(mvp);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(false);
        batch.Grid(10, 1.0f, true);
        batch.Render();

        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        batch.SetMatrix(ortho);
        batch.SetColor(255, 255, 255);
        //
        font.Print(10, 10, "Fps :%d", device.GetFPS());

        int Y = 22;

        font.Print(10, Y * 5, " Camera pos :%f %f %f", cameraPos.x, cameraPos.y, cameraPos.z);

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
