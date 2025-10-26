

#include "Core.hpp"
 

int screenWidth = 1024;
int screenHeight = 768;




 
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


    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;

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
        

       driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);


       batch.SetMatrix(mvp);
       driver.SetDepthTest(true);
       driver.SetBlendEnable(false);

       batch.Grid(10, 1.0f, true);

       batch.Render();
 


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
