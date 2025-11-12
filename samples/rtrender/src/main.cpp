#include "Core.hpp"

// ============================================
// GLOBAL SETTINGS
// ============================================
int screenWidth = 1024;
int screenHeight = 768;

// ============================================
// QUAD RENDERER HELPER
// ============================================
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

// ============================================
// SHADER SOURCE CODE
// ============================================

// Scene shader (with lighting)
const char *sceneVertexShader = R"(
#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char *sceneFragmentShader = R"(
#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{           
    vec3 color = texture(diffuseTexture, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(1.0);
    
    // Ambient
    vec3 ambient = 0.3 * lightColor;
    
    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor * 0.3;
    
    vec3 lighting = (ambient + diffuse + specular) * color;    
    FragColor = vec4(lighting, 1.0);
}
)";

// Mirror/Water shader with Fresnel
const char *mirrorVertexShader = R"(
#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char *mirrorFragmentShader = R"(
#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D u_reflectionTexture;
uniform vec3 u_viewPos;
uniform float u_fresnelPower;
uniform float u_fresnelMin;
uniform vec3 u_mirrorTint;
uniform float u_time;
uniform float u_waveStrength;
uniform bool u_isWater;

void main()
{           
    vec3 N = normalize(Normal);
    vec3 V = normalize(u_viewPos - FragPos);
    
    // Water distortion (optional)
    vec2 distortion = vec2(0.0);
    if (u_isWater)
    {
        distortion = vec2(
            sin(TexCoords.y * 15.0 + u_time * 2.0) * u_waveStrength,
            cos(TexCoords.x * 15.0 + u_time * 2.0) * u_waveStrength
        );
    }
    
    vec2 finalUV = TexCoords + distortion;
    
    // Fresnel effect
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - NdotV, u_fresnelPower);
    fresnel = mix(u_fresnelMin, 1.0, fresnel);
    
    // Sample reflection
    vec3 reflection = texture(u_reflectionTexture, finalUV).rgb;
    
    // Base color
    vec3 baseColor = u_mirrorTint * 0.1;
    if (u_isWater)
    {
        baseColor = vec3(0.1, 0.3, 0.4); // Water color
    }
    
    // Mix
    vec3 color = mix(baseColor, reflection, fresnel);
    
    // Specular highlight
    vec3 specular = vec3(pow(NdotV, 50.0)) * 0.2;
    color += specular;
    
    float alpha = u_isWater ? 0.9 : 1.0;
    FragColor = vec4(color, alpha);
}
)";

const char *debugQuadVS = R"(
#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}
)";

const char *debugQuadFS = R"(
#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tex;
void main() {
    FragColor = texture(tex, TexCoord);
}
)";

Vec3 ReflectPointAcrossPlane(const Vec3 &point, const Vec3 &planePoint, const Vec3 &planeNormal)
{
    Vec3 diff = point - planePoint;
    float distance = Vec3::Dot(diff, planeNormal);
    return point - planeNormal * (2.0f * distance);
}

// ============================================
// MAIN APPLICATION
// ============================================
int main()
{
    // ============================================
    // INITIALIZATION
    // ============================================
    Device &device = Device::Instance();
    if (!device.Create(screenWidth, screenHeight, "Advanced Mirror & Water Demo", true, 1))
    {
        LogError("Failed to create device!");
        return 1;
    }

    Driver &driver = Driver::Instance();
    driver.SetClearDepth(1.0f);
    driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    driver.SetViewPort(0, 0, screenWidth, screenHeight);

    // Batch renderer for UI
    RenderBatch batch;
    batch.Init();

    // Font for UI
    Font font;
    font.SetBatch(&batch);
    font.LoadDefaultFont();

    // GUI
    GUI gui;
    gui.Init(&batch, &font);

    // ============================================
    // CAMERA SETUP
    // ============================================
    CameraFree camera(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
    camera.setPosition(0.0f, 3.0f, 10.0f);

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;
    float mouseSensitivity = 0.8f;

    // ============================================
    // SHADER CREATION
    // ============================================
    Shader *sceneShader = ShaderManager::Instance().Create("scene", sceneVertexShader, sceneFragmentShader);
    Shader *mirrorShader = ShaderManager::Instance().Create("mirror", mirrorVertexShader, mirrorFragmentShader);
    Shader *debugShader = ShaderManager::Instance().Create("debug", debugQuadVS, debugQuadFS);

    if (!sceneShader || !mirrorShader || !debugShader)
    {
        LogError("Failed to create shaders!");
        return 1;
    }

    QuadRenderer quad;

    // ============================================
    // TEXTURE LOADING
    // ============================================
    TextureManager::Instance().SetLoadPath("../assets/");
    TextureManager::Instance().Add("wall.jpg", true);
    TextureManager::Instance().Add("marm.jpg", true);
    TextureManager::Instance().Add("sinbad/sinbad_body.tga", false);
    TextureManager::Instance().Add("sinbad/sinbad_clothes.tga", false);
    TextureManager::Instance().Add("sinbad/sinbad_sword.tga", false);

    // ============================================
    // MESH CREATION
    // ============================================

    // Ground plane
    // Mesh *plane = MeshManager::Instance().CreatePlane("plane", 20, 20, 50, 50, 10, 10);
    // plane->AddMaterial("marm");
    // plane->GetMaterial(0)->SetTexture(0, TextureManager::Instance().Get("marm"));

    // Cubes
    Mesh *cube = MeshManager::Instance().CreateCube("cube");
    cube->AddMaterial("wall");
    cube->GetMaterial(0)->SetTexture(0, TextureManager::Instance().Get("wall"));

    // Mirror (vertical, facing right)
    Mesh *mirror = MeshManager::Instance().CreateQuad("mirror", Vec3(1, 0, 0), 5.0f);
    mirror->AddMaterial("reflection");

    // Water (horizontal)
    Mesh *water = MeshManager::Instance().CreateQuad("water", Vec3(0, 1, 0), 10.0f);
    water->AddMaterial("reflection");

    // Character model
    Mesh *meshModel = MeshManager::Instance().Load("sinbad", "../assets/sinbad/sinbad.h3d");

    if (meshModel)
    {
        Material *material = meshModel->AddMaterial("body");
        material->SetTexture(0, TextureManager::Instance().Get("sinbad_body"));
        material = meshModel->AddMaterial("clothes");
        material->SetTexture(0, TextureManager::Instance().Get("sinbad_clothes"));
        material = meshModel->AddMaterial("sword");
        material->SetTexture(0, TextureManager::Instance().Get("sinbad_sword"));

        meshModel->SetBufferMaterial(0, 1);
        meshModel->SetBufferMaterial(1, 1);
        meshModel->SetBufferMaterial(2, 2);
        meshModel->SetBufferMaterial(3, 1);
        meshModel->SetBufferMaterial(4, 3);
        meshModel->SetBufferMaterial(5, 2);
        meshModel->SetBufferMaterial(6, 2);
    }

    // ============================================
    // ANIMATOR SETUP
    // ============================================
    Animator animator(meshModel);

    AnimationLayer *torsoLayer = animator.AddLayer();
    torsoLayer->LoadAnimation("topRun", "../assets/sinbad/sinbad_RunTop.anim");
    torsoLayer->Play("topRun", PlayMode::Loop);

    AnimationLayer *legsLayer = animator.AddLayer();
    legsLayer->LoadAnimation("legsRun", "../assets/sinbad/sinbad_RunBase.anim");
    legsLayer->Play("legsRun", PlayMode::Loop);

    // ============================================
    // RENDER TARGET CREATION
    // ============================================
    auto &rtMgr = RenderTargetManager::Instance();

    // RenderTarget *mirrorRT = rtMgr.CreateHDR("MirrorReflection", 1024, 1024);

    RenderTarget *mirrorRT = rtMgr.Create("MirrorReflection", 1024, 1024);
    mirrorRT->AddColorAttachment(TextureFormat::RGBA8);
    mirrorRT->AddDepthAttachment(TextureFormat::DEPTH24);
    if (!mirrorRT->Finalize())
    {
        LogError("Failed to finalize mirror RT!");
        mirrorRT->PrintInfo();
        return 1;
    }

    RenderTarget *waterRT = rtMgr.Create("WaterReflection", 1024, 1024);
    waterRT->AddColorAttachment(TextureFormat::RGBA8);
    waterRT->AddDepthAttachment(TextureFormat::DEPTH24);
    if (!waterRT->Finalize())
    {
        LogError("Failed to finalize mirror RT!");
        waterRT->PrintInfo();
        return 1;
    }

    // RenderTarget *waterRT = rtMgr.CreateHDR("WaterReflection", 1024, 1024);

    if (!mirrorRT || !waterRT)
    {
        LogError("Failed to create render targets!");
        return 1;
    }

    rtMgr.PrintStats();

    // ============================================
    // SCENE SETUP
    // ============================================
    Vec3 lightPos(-2.0f, 8.0f, -4.0f);

    Vec3 cubePositions[] = {
        Vec3(0.0f, 0.5f, 0.0f),
        Vec3(3.0f, 0.5f, -3.0f),
        Vec3(-3.0f, 0.5f, -3.0f),
        Vec3(5.0f, 0.5f, 2.0f),
        Vec3(-5.0f, 0.5f, 2.0f),
        Vec3(2.0f, 0.5f, -6.0f),
        Vec3(-2.0f, 0.5f, -6.0f),
    };

    // ============================================
    // MIRROR & WATER SETTINGS
    // ============================================

    // Mirror settings
    Vec3 mirrorPosition(-8.0f, 2.5f, 0.0f);
    Vec3 mirrorNormal(1.0f, 0.0f, 0.0f); // Normal apontando para +X

    
    // Water settings
    Vec3 waterPosition(0.0f, 0.0f, 0.0f);
    Vec3 waterNormal(0.0f, 1.0f, 0.0f); // Normal apontando para +Y

    

    // Material properties
    float mirrorFresnelPower = 3.0f;
    float mirrorFresnelMin = 0.05f;
    Vec3 mirrorTint(0.9f, 0.95f, 1.0f);

    float waterFresnelPower = 2.0f;
    float waterFresnelMin = 0.1f;
    float waterWaveStrength = 0.008f;

    // UI state
    bool showMirror = true;
    bool showWater = true;
    bool animateLight = false;

    // ============================================
    // MAIN LOOP
    // ============================================
    LogInfo("Entering main loop...");

    while (device.IsRunning())
    {
        float dt = device.GetFrameTime();
        float time = (float)device.GetTime();
        const float SPEED = 8.0f * dt;

        // Animate light (optional)
        if (animateLight)
        {
            lightPos.x = sin(time) * 6.0f;
            lightPos.z = cos(time) * 6.0f;
        }

        // ============================================
        // INPUT HANDLING
        // ============================================
        Input::Update();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                device.SetShouldClose(true);
                break;
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    screenWidth = event.window.data1;
                    screenHeight = event.window.data2;
                    driver.SetViewPort(0, 0, screenWidth, screenHeight);
                    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
                }
            }
            case SDL_MOUSEBUTTONDOWN:
                Input::OnMouseDown(event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                Input::OnMouseUp(event.button);
                break;
            case SDL_MOUSEMOTION:
                Input::OnMouseMove(event.motion);
                break;
            case SDL_MOUSEWHEEL:
                Input::OnMouseWheel(event.wheel);
                break;
            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    device.SetShouldClose(true);
                    break;
                }
                Input::OnKeyDown(event.key);
                break;
            }
            case SDL_KEYUP:
                Input::OnKeyUp(event.key);
                break;
            case SDL_TEXTINPUT:
            {
                Input::OnTextInput(event.text);
                break;
            }
            }
        }

        // Mouse camera control
        int xposIn, yposIn;
        u32 IsMouseDown = SDL_GetMouseState(&xposIn, &yposIn);

        if (IsMouseDown & SDL_BUTTON(SDL_BUTTON_RIGHT))
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

            camera.rotate(yoffset * mouseSensitivity, xoffset * mouseSensitivity);
        }
        else
        {
            firstMouse = true;
        }

        // Keyboard camera movement
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W])
            camera.move(SPEED);
        if (state[SDL_SCANCODE_S])
            camera.move(-SPEED);
        if (state[SDL_SCANCODE_A])
            camera.strafe(-SPEED);
        if (state[SDL_SCANCODE_D])
            camera.strafe(SPEED);
        if (state[SDL_SCANCODE_Q])
            camera.setPosition(camera.getPosition() + Vec3(0, -SPEED, 0));
        if (state[SDL_SCANCODE_E])
            camera.setPosition(camera.getPosition() + Vec3(0, SPEED, 0));

        camera.update(1.0f);
        animator.Update(dt);

        driver.SetCulling(CullMode::Back);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        // ============================================
        // RENDER MIRROR REFLECTION
        // ============================================
        if (showMirror)
        {
         
            Vec3 mirrorCameraPos = mirrorPosition;               // Câmera NO espelho
            Vec3 mirrorLookDir = Vec3(-1, 0, 0);                 // Olha para TRÁS (oposto da normal)
            Vec3 mirrorTarget = mirrorCameraPos - mirrorLookDir; // Para onde olha
            Vec3 mirrorUp = Vec3(0, 1, 0);                       // Up normal

            Mat4 reflectionView = Mat4::LookAt(mirrorCameraPos, mirrorTarget, mirrorUp);

            //  reflectionView = reflectionView.inverse();

            //   Mat4 reflectionView =Mat4::LookAt(Vec3(0.0f, 5.0f, 10.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

            Mat4 reflectionProj = Mat4::Perspective(45.0f, mirrorRT->GetWidth() / (float)mirrorRT->GetHeight(), 0.1f, 1000.0f);

            // Bind render target
            mirrorRT->Bind();
            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
            // driver.SetCulling(CullMode::Front);

            sceneShader->Bind();
            sceneShader->SetUniform("diffuseTexture", 0);
            sceneShader->SetUniformMat4("projection", reflectionProj.m);
            sceneShader->SetUniformMat4("view", reflectionView.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", mirrorCameraPos.x, mirrorCameraPos.y, mirrorCameraPos.z);

            // Render objetos (exceto o próprio mirror)
            Mat4 model;

            for (const auto &pos : cubePositions)
            {

                model = Mat4::Translation(pos);
                sceneShader->SetUniformMat4("model", model.m);
                driver.DrawMesh(cube);
            }

            if (meshModel)
            {
                Vec3 modelPos(-2.0f, 0.5f, 0.0f);

                model = Mat4::Translation(modelPos) * Mat4::Scale(Vec3(0.1f));
                sceneShader->SetUniformMat4("model", model.m);
                driver.DrawMesh(meshModel);
            }

            mirrorRT->Unbind();
        }

        driver.SetCulling(CullMode::Back);

        // ============================================
        // RENDER WATER REFLECTION
        // ============================================

        if (showWater)
        {
              Vec3 waterCamPos = waterPosition;
  
  
       

            Vec3 waterCameraPos = Vec3(0,-50.0f,0.01f);               // Câmera NO espelho
            Vec3 waterLookDir = Vec3(0, 1, 0);                 // Olha para TRÁS (oposto da normal)
            Vec3 waterTarget =   waterLookDir; // Para onde olha
            Vec3 waterUp = Vec3(0, 1, 0);                       // Up normal

            Mat4 reflectionView = Mat4::LookAt(waterCameraPos, waterTarget, waterUp);
                

  //              Mat4 reflectionView = Mat4::LookAt(Vec3(0.1f, 5.0f, 0.1f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f));
            Mat4 reflectionProj = Mat4::Perspective(45.0f, 1.0f, 0.1f, 100.0f);

            waterRT->Bind();
            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

            sceneShader->Bind();
            sceneShader->SetUniform("diffuseTexture", 0);
            sceneShader->SetUniformMat4("projection", reflectionProj.m);
            sceneShader->SetUniformMat4("view", reflectionView.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", waterCamPos.x, waterCamPos.y, waterCamPos.z);

            // Render objetos ACIMA da água
            Mat4 model;
            for (const auto &pos : cubePositions)
            {

                model = Mat4::Translation(pos);
                sceneShader->SetUniformMat4("model", model.m);
                driver.DrawMesh(cube);
            }

            if (meshModel)
            {
                Vec3 modelPos(-2.0f, 0.5f, 0.0f);

                model = Mat4::Translation(modelPos) * Mat4::Scale(Vec3(0.1f));
                sceneShader->SetUniformMat4("model", model.m);
                driver.DrawMesh(meshModel);
            }

            waterRT->Unbind();
        }

        // ============================================
        // RENDER MAIN SCENE
        // ============================================
        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        Vec3 cameraPos = camera.getPosition();
        Mat4 view = camera.getViewMatrix();
        Mat4 proj = camera.getProjectionMatrix();

        sceneShader->Bind();
        sceneShader->SetUniform("diffuseTexture", 0);
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        // Draw ground
        Mat4 model;

        // Draw cubes
        for (const auto &pos : cubePositions)
        {
            model = Mat4::Translation(pos);
            sceneShader->SetUniformMat4("model", model.m);
            driver.DrawMesh(cube);
        }

        // Draw character
        if (meshModel)
        {
            model = Mat4::Translation(Vec3(-2.0f, 0.5f, 0.0f)) * Mat4::Scale(Vec3(0.1f));
            sceneShader->SetUniformMat4("model", model.m);
            driver.DrawMesh(meshModel);
        }

        // ============================================
        // RENDER MIRROR
        // ============================================
        Texture *mirrorTexture = mirrorRT->GetColorTexture(0);
        if (showMirror)
        {
            mirror->SetTexture(0, mirrorTexture);

            mirrorShader->Bind();
            mirrorShader->SetUniform("u_reflectionTexture", 0);
            mirrorShader->SetUniformMat4("projection", proj.m);
            mirrorShader->SetUniformMat4("view", view.m);
            mirrorShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            mirrorShader->SetUniform("u_fresnelPower", mirrorFresnelPower);
            mirrorShader->SetUniform("u_fresnelMin", mirrorFresnelMin);
            mirrorShader->SetUniform("u_mirrorTint", mirrorTint.x, mirrorTint.y, mirrorTint.z);
            mirrorShader->SetUniform("u_time", time);
            mirrorShader->SetUniform("u_waveStrength", 0.0f);
            mirrorShader->SetUniform("u_isWater", false);

            model = Mat4::Translation(mirrorPosition);
            mirrorShader->SetUniformMat4("model", model.m);
            driver.DrawMesh(mirror);
        }

        // ============================================
        // RENDER WATER
        // ============================================
        Texture *waterTexture = waterRT->GetColorTexture(0);
        if (showWater)
        {
            water->SetTexture(0, waterTexture);

            driver.SetBlendEnable(true);
            driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

            mirrorShader->Bind();
            mirrorShader->SetUniform("u_reflectionTexture", 0);
            mirrorShader->SetUniformMat4("projection", proj.m);
            mirrorShader->SetUniformMat4("view", view.m);
            mirrorShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            mirrorShader->SetUniform("u_fresnelPower", waterFresnelPower);
            mirrorShader->SetUniform("u_fresnelMin", waterFresnelMin);
            mirrorShader->SetUniform("u_mirrorTint", 0.1f, 0.3f, 0.4f);
            mirrorShader->SetUniform("u_time", time);
            mirrorShader->SetUniform("u_waveStrength", waterWaveStrength);
            mirrorShader->SetUniform("u_isWater", true);

            model = Mat4::Translation(waterPosition) * Mat4::Scale(Vec3(2.0f));
            mirrorShader->SetUniformMat4("model", model.m);
            driver.DrawMesh(water);

            driver.SetBlendEnable(false);
        }

        // ============================================
        // RENDER UI
        // ============================================
        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
        batch.SetMatrix(ortho);

        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

        // Main settings window
        gui.BeginWindow("Settings", 10, 10, 280, 550);

        float y = 10;

        // Toggle controls
        gui.Text(10, y, "Display");
        y += 20;
        gui.Checkbox("Show Mirror", &showMirror, 10, y, 20);
        y += 25;
        gui.Checkbox("Show Water", &showWater, 10, y, 20);
        y += 25;
        gui.Checkbox("Animate Light", &animateLight, 10, y, 20);

        // Mirror settings
        y += 40;
        gui.Text(10, y, "Mirror Settings");
        y += 20;
        gui.SliderFloat("Fresnel Power", &mirrorFresnelPower, 0.0f, 10.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Fresnel Min", &mirrorFresnelMin, 0.0f, 1.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Pos X", &mirrorPosition.x, -15.0f, 15.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Pos Y", &mirrorPosition.y, -5.0f, 10.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Pos Z", &mirrorPosition.z, -15.0f, 15.0f, 10, y, 260, 20);
 

        // Water settings
        y += 40;
        gui.Text(10, y, "Water Settings");
        y += 20;
        gui.SliderFloat("W Fresnel Pow", &waterFresnelPower, 0.0f, 10.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("W Fresnel Min", &waterFresnelMin, 0.0f, 1.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Wave Strength", &waterWaveStrength, 0.0f, 0.05f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("W Height", &waterPosition.y, -2.0f, 2.0f, 10, y, 260, 20);
 

        // Light settings
        y += 40;
        gui.Text(10, y, "Light Position");
        y += 20;
        gui.SliderFloat("L X", &lightPos.x, -10.0f, 10.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("L Y", &lightPos.y, 0.0f, 20.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("L Z", &lightPos.z, -10.0f, 10.0f, 10, y, 260, 20);

        gui.EndWindow();

        // Stats window
        gui.BeginWindow("Stats", screenWidth - 250, 10, 260, 180);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "FPS: %.1f", 1.0f / dt);
        gui.Text(10, 10, buffer);
        snprintf(buffer, sizeof(buffer), "Frame Time: %.2f ms", dt * 1000.0f);
        gui.Text(10, 30, buffer);
        snprintf(buffer, sizeof(buffer), "Camera: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
        gui.Text(10, 50, buffer);
        gui.Text(10, 80, "Controls:");
        gui.Text(10, 95, "WASD - Move | QE - Up/Down");
        gui.EndWindow();

        gui.EndFrame();
        batch.Render();

        driver.SetBlendEnable(false);

        debugShader->Bind();
        debugShader->SetUniform("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mirrorTexture->GetHandle());
        quad.render(210, 0, 200, 200, screenWidth, screenHeight);

        debugShader->SetUniform("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterTexture->GetHandle());
        quad.render(420, 0, 200, 200, screenWidth, screenHeight);

        // ============================================
        // SWAP BUFFERS
        // ============================================
        device.Flip();
        driver.Reset();
    }

    // ============================================
    // CLEANUP
    // ============================================
    LogInfo("Shutting down...");

    font.Release();
    batch.Release();



    device.Close();

    LogInfo("Goodbye!");
    return 0;
}
