

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

const char *shadowVertexShader = R"(
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
 
 

uniform sampler2D diffuse;
 

uniform vec3 lightPos;
uniform vec3 viewPos;

 

void main()
{           
    vec3 color = texture(diffuse, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(1.0,0.0,0.0);
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
                    
    vec3 lighting = (ambient  + diffuse + specular) * color;    
    
    FragColor = vec4(lighting, 1.0);
}

)";

void aPrintMatrix(const Mat4 &mat, const std::string &title)
{
    std::cout << title << std::endl;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            std::cout << mat(i, j) << " ";
        }
        std::cout << std::endl;
    }
}

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

    Mesh *model = MeshManager::Instance().Load("modle", "assets/sinbad/sinbad.h3d");
 
    //animation.Load("assets/sinbad/sinbad_JumpLoop.anim");
    //animation.Load("assets/sinbad/sinbad_IdleTop.anim");
    //animation.Load("assets/sinbad/sinbad_DrawSwords.anim");
    //animation.Load("assets/sinbad/sinbad_Dance.anim");
   
    //animation.BindToMesh(model);
    
 

    //animationTop.BindToMesh(model);

    Animator animator =Animator(model);
    animator.LoadAnimation("topIdle", "assets/sinbad/sinbad_IdleTop.anim");
    animator.LoadAnimation("topRun", "assets/sinbad/sinbad_RunTop.anim");
    animator.LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");


    u32 upperLayer = animator.AddLayer("Upper", 0.5f);
    u32 lowerLayer = animator.AddLayer("Lower", 0.5f);

    animator.PlayOnLayer(upperLayer, "topRun", PlayMode::Loop);
    animator.PlayOnLayer(lowerLayer, "legsRun", PlayMode::Loop);

  //  animator.Play("IdleTop", PlayMode::Loop);
    //animator.Play("RunBase", PlayMode::Loop);


     Mesh *modelDefault = MeshManager::Instance().Load("default", "assets/anim.h3d");
     Animation defaultIdle;
     defaultIdle.Load("assets/idle.anim");
     defaultIdle.BindToMesh(modelDefault);



    Shader *shader = ShaderManager::Instance().Create("shader", shadowVertexShader, shadowFragmentShader);

    Mesh *cube = MeshManager::Instance().CreateCube("cube");

    Texture *texture = TextureManager::Instance().Get("checker");

    bool firstMouse{true};
    float mouseSensitivity{0.8f};

    Vec3 lightPos(-2, 4.0f, -4.0f);


    float totalTime = 0.0f;
    

    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        totalTime +=  dt;
        const float SPEED = 12.0f * dt;

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
        const Vec3 cameraPos = camera.getPosition();
        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        shader->Bind();
        shader->SetUniformMat4("projection", proj.m);
        shader->SetUniformMat4("view", view.m);
        shader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        shader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        shader->SetTexture2D("diffuse", texture->GetHandle(), 0);

        u32 count = model->GetBoneCount();
       
          
 
        // animation.Update(dt  );
        // animationTop.Update(dt  );
        defaultIdle.Update(dt  );

        animator.Update(dt);


        float S=0.2f;

        Mat4 matModel=Mat4::Translation(0.5f, 0.9f, 0.0f) * Mat4::Scale(S, S, S);
        shader->SetUniformMat4("model", matModel.m );

        driver.DrawMesh(model);

        S=0.01f;
        matModel= Mat4::Translation(-0.5f, 0.0f, 0.0f) * Mat4::Scale(S, S, S) ;
        shader->SetUniformMat4("model", matModel.m );
        driver.DrawMesh(modelDefault);

 
        // for (u32 i = 0; i < count; i++)
        // {
        //     Bone *bone = model->GetBone(i);

        // //     Mat4 bindPose = Mat4::Inverse(bone->inverseBindPose);

      
            

        // //     Mat4 final =  matModel * bindPose ;
         
         

        // //     shader->SetUniformMat4("model", final.m );
        // //    cube->Render();


        //     //Mat4 mat = matModel * bone->GetGlobalTransform();
        //   //  shader->SetUniformMat4("model", mat.m );
        //    // cube->Render();


        
        // }



        batch.SetMatrix(mvp);
        batch.Grid(10, 1.0f, true);

        batch.BeginTransform(matModel);
        //model->Debug(&batch);
        batch.EndTransform();


 

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        batch.SetColor(255, 255, 255);
        //
        font.Print(10, 10, "Fps :%d", device.GetFPS());

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
