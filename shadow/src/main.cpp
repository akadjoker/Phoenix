

#include "Core.hpp"

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
out vec4 FragColor;
void main()
{             
     //gl_FragDepth = gl_FragCoord.z;
    FragColor = vec4(0.1); 
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
uniform float near_plane;
uniform float far_plane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
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
out vec4 FragPosLightSpace;


uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
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
in vec4 FragPosLightSpace;
 

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(1024.0,1024.0);//textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{           
    vec3 color = texture(diffuseTexture, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * lightColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
}

)";

int main()
{

    Device &device = Device::Instance();

    if (!device.Create(screenWidth, screenHeight, "Shadow", true, 1))
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

    //******************************************* */

    Shader *simpleDepthShader = ShaderManager::Instance().Create("depth", depthVertexShader, depthFragmentShader);
    Shader *debugShader = ShaderManager::Instance().Create("debug", debugVertexShader, debugFragmentShader);
    Shader *shader = ShaderManager::Instance().Create("shader", shadowVertexShader, shadowFragmentShader);

    // configure depth map FBO
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    // Create framebuffer
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // Create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
 
    unsigned int dummyColor;
    glGenTextures(1, &dummyColor);
    glBindTexture(GL_TEXTURE_2D, dummyColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Attach both to framebuffer
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dummyColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    // Check status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("ERROR::FRAMEBUFFER:: Shadow framebuffer is not complete!");
    }

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LogError("OpenGL Error: %d", error);
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

    TextureManager::Instance().SetLoadPath("assets/");
    TextureManager::Instance().Add("wall.jpg",true);
    TextureManager::Instance().Add("marm.jpg",true);



    TextureManager::Instance().Add("sinbad/sinbad_body.tga",false);
    TextureManager::Instance().Add("sinbad/sinbad_clothes.tga",false);
    TextureManager::Instance().Add("sinbad/sinbad_sword.tga",false);
    

    Mesh *meshModel = MeshManager::Instance().Load("sinbad", "assets/sinbad/sinbad.h3d");

    Material *material = meshModel->AddMaterial("body");
    material->SetTexture(0, TextureManager::Instance().Get("sinbad_body"));
    material = meshModel->AddMaterial("clothes");
    material->SetTexture(0, TextureManager::Instance().Get("sinbad_clothes"));
    material = meshModel->AddMaterial("sword");
    material->SetTexture(0, TextureManager::Instance().Get("sinbad_sword"));

    meshModel->SetBufferMaterial(0, 1);//olhos
    meshModel->SetBufferMaterial(1, 1);//tronco
    meshModel->SetBufferMaterial(2, 2);//rings
    meshModel->SetBufferMaterial(3, 1);
    meshModel->SetBufferMaterial(4, 3);//espada
    meshModel->SetBufferMaterial(5, 2);
    meshModel->SetBufferMaterial(6, 2);//pernas

    Animator animator =Animator(meshModel);

    AnimationLayer *torsoLayer = animator.AddLayer();

    torsoLayer->LoadAnimation("topIdle", "assets/sinbad/sinbad_IdleTop.anim");
    torsoLayer->LoadAnimation("topSliceHorizontal", "assets/sinbad/sinbad_SliceHorizontal.anim");
    torsoLayer->LoadAnimation("topSliceVertical", "assets/sinbad/sinbad_SliceVertical.anim");
    torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_RunTop.anim");
    torsoLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");
    torsoLayer->Play("topRun", PlayMode::Loop);

    AnimationLayer *legsLayer = animator.AddLayer();
    legsLayer->LoadAnimation("legsIdle", "assets/sinbad/sinbad_IdleBase.anim");
    legsLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");
    legsLayer->Play("legsRun", PlayMode::Loop);
    


    Mesh *plane = MeshManager::Instance().CreatePlane("plane", 10, 10, 50, 50,20,20);
    plane->AddMaterial("marm");
    plane->GetMaterial(0)->SetTexture(0, TextureManager::Instance().Get("marm"));


    Mesh *cube = MeshManager::Instance().CreateCube("cube");
    cube->AddMaterial("wall");
    cube->GetMaterial(0)->SetTexture(0, TextureManager::Instance().Get("wall"));



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
        const Vec3 cameraPos = camera.getPosition();

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);


        animator.Update(dt);
 
        driver.SetCulling(CullMode::None);
        driver.SetDepthTest(true);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render depth of scene to texture (from light's perspective)
        Mat4 lightProjection, lightView;
        Mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 50.0f; // ← Aumenta far plane
        lightProjection = Mat4::Ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        lightView = Mat4::LookAt(lightPos, Vec3(0.0f), Vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        simpleDepthShader->Bind();
        simpleDepthShader->SetUniformMat4("lightSpaceMatrix", lightSpaceMatrix.m);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO));
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        Mat4 model;

        // Render cubes
        for (int i = 0; i < 10; i++)
        {
            model = Mat4::Translation(cubePositions[i]);
            simpleDepthShader->SetUniformMat4("model", model.m);
            driver.DrawMesh(cube);
        }
        float S=0.2f;
        Mat4 matModel=Mat4::Translation(-2.5f, 0.9f, -0.5f) * Mat4::Scale(S, S, S);
        simpleDepthShader->SetUniformMat4("model", matModel.m );
        driver.DrawMesh(meshModel);
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));

   
        // reset viewport
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);
        // 2. render scene as normal using the generated depth/shadow map
        shader->Bind();
        shader->SetUniform("diffuseTexture", 0);
        shader->SetUniform("shadowMap", 1);
        shader->SetUniformMat4("projection", proj.m);
        shader->SetUniformMat4("view", view.m);
        shader->SetUniformMat4("lightSpaceMatrix", lightSpaceMatrix.m);
        shader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        shader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        shader->SetTexture2D("shadowMap", depthMap, 1);
        shader->SetTexture2D("diffuseTexture", 0, 0);

        model = Mat4::Scale(Vec3(2.0f));
        shader->SetUniformMat4("model", model.m);
        driver.DrawMesh(plane);
        for (int i = 0; i < 10; i++)
        {
            model = Mat4::Translation(cubePositions[i]) * Mat4::Scale(Vec3(1.0f));
            shader->SetUniformMat4("model", model.m);
            driver.DrawMesh(cube);
        }
        shader->SetUniformMat4("model", matModel.m );
        driver.DrawMesh(meshModel);

        // render Depth map to quad for visual debugging
     //   driver.SetDepthTest(false);
        driver.SetBlendEnable(false);
        debugShader->Bind();
        debugShader->SetUniform("depthMap", 0);
        debugShader->SetUniform("near_plane", near_plane);
        debugShader->SetUniform("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, depthMap));
        quad.render(0, 0, 200, 200, screenWidth, screenHeight);
 

        //    batch.SetMatrix(mvp);

        //    batch.Grid(10, 1.0f, true);

        //    batch.Render();

        //    batch.SetMatrix(ortho);
        //    driver.SetBlendEnable(true);
        //    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        //    batch.SetColor(255, 255, 255);
        //    font.Print(10,10,"Fps :%d",device.GetFPS());
        //    batch.Render();

        device.Flip();
        driver.Reset();
    }

    font.Release();
    batch.Release();

    MeshManager::Instance().UnloadAll();
    ShaderManager::Instance().UnloadAll();
    TextureManager::Instance().UnloadAll();
    device.Close();

    return 0;
}
