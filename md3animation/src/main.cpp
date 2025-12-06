

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

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

    GUI gui;
    gui.Init(&batch, &font);

    FreeCameraComponent *cameraMove;
    Camera camera;

    cameraMove = camera.addComponent<FreeCameraComponent>();
    cameraMove->setMoveSpeed(15.0f);
    cameraMove->setMouseSensitivity(0.15f);
    cameraMove->setSprintMultiplier(3.0f);
    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
    camera.setFOV(45.0f);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);

    Utils::ChangeDirectory("../");
    Shader *sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
    if (!sceneShader)
    {
        LogError("Failed to load scene shader");
        return false;
    }


  
 
    


    MeshM3D *berreta = MeshManager::Instance().LoadM3D("Berreta","assets/md3/beretta/beretta_hold.md3" );

    if (!berreta)
    {
        LogError("Failed to load berreta");
        return false;
    }

 

    MeshM3D *specs[8];

    specs[0] = MeshManager::Instance().LoadM3D("Main","assets/md3/beretta/beretta_view_main.md3" );
    specs[1] = MeshManager::Instance().LoadM3D("Slide","assets/md3/beretta/beretta_view_slide.md3" );
    specs[2] = MeshManager::Instance().LoadM3D("Clip","assets/md3/beretta/beretta_view_clip.md3");
    specs[3] = MeshManager::Instance().LoadM3D("Safety","assets/md3/beretta/beretta_view_safety.md3");
    specs[4] = MeshManager::Instance().LoadM3D("Sliderel","assets/md3/beretta/beretta_view_sliderel.md3");
    specs[5] = MeshManager::Instance().LoadM3D("cock","assets/md3/beretta/beretta_view_cock.md3");
    specs[6] = MeshManager::Instance().LoadM3D("trigger","assets/md3/beretta/beretta_view_trigger.md3");


    
 

    TextureManager::Instance().SetLoadPath("assets/");
    TextureManager::Instance().SetFlipVerticalOnLoad(false);
    
    Material *mat = berreta->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/hand_free.tga"));
    berreta->SetSurfaceMaterial(0, 0);
    berreta->SetSurfaceMaterial(1, 0);


    mat = specs[0]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[1]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[2]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[3]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[4]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[5]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    mat = specs[6]->GetMaterial(0);
    mat->SetTexture(0, TextureManager::Instance().Add("md3/beretta/beretta.tga"));
    
    GameObject mainGgun("gun");
   // mainGgun.setParent(&camera);

    GameObject gun("gun");
    gun.setParent(&mainGgun);
    gun.addComponent<MeshM3DRenderer>(berreta);


    VertexAnimator *animator = gun.addComponent<VertexAnimator>();
    animator->AddAnimation("draw", 0, 54,15, false);
    animator->AddAnimation("idle", 44,45,10, true);
    animator->AddAnimation("shoot", 54,54+10,26, false);
    animator->AddAnimation("reload", 75,75 +48,28, false);
    animator->PlayAnimationThen("draw", "idle", false);


    GameObject gunMain("gunMain");
    gunMain.setParent(gun.getJoint(7));
    gunMain.addComponent<MeshM3DRenderer>(specs[0]);
//    gunMain.scale(Vec3(0.2f));

    GameObject gunSlide("gunSlide");
    gunSlide.setParent(gun.getJoint(5));
    gunSlide.addComponent<MeshM3DRenderer>(specs[1]);
    gunSlide.scale(Vec3(0.2f));
    gunSlide.translate(Vec3(0.0f, 0.0f,-1.4f));


    GameObject gunClip("gunClip");
    gunClip.setParent(gun.getJoint(2));
    gunClip.addComponent<MeshM3DRenderer>(specs[2]);
    gunClip.scale(Vec3(0.3f));
    gunClip.translate(Vec3(0.0f, 0.0f,-1.5f));

    
    GameObject gunSafety("gunSafety");
    gunSafety.setParent(gun.getJoint(4));
    gunSafety.addComponent<MeshM3DRenderer>(specs[3]);
    

    
    GameObject sliderel("sliderel");
    sliderel.setParent(gun.getJoint(1));
    sliderel.addComponent<MeshM3DRenderer>(specs[4]);

    

    
    GameObject cock("cock");
    cock.setParent(gun.getJoint(0));
    cock.addComponent<MeshM3DRenderer>(specs[5]);

    
    
    GameObject trigger("trigger");
    trigger.setParent(gun.getJoint(3));
    trigger.addComponent<MeshM3DRenderer>(specs[6]);
    


        //  Tag: tag_cock (0)
        //  Tag: tag_sliderel (1)
        //  Tag: tag_clip (2)
        //  Tag: tag_trigger (3)
        //  Tag: tag_safety (4)
        //  Tag: tag_slide (5)
        //  Tag: tag_eject (6)
        //  Tag: tag_main (7)
        //  Tag: tag_laser (8)
        //  Tag: tag_flash (9)
        //  Tag: tag_weapon (10)
    




    gun.scale(Vec3(0.1f));
    gun.rotateDeg(Vec3(0.0f, 0.0f, 1.0f), 90.0f);
    gun.rotateDeg(Vec3(0.0f, 1.0f, 0.0f), 90.0f);
    gun.setPosition(Vec3(0.0f, -0.08f, -0.3f));

 

    int shots = 0; 

    while (device.Run())
    {

        float dt = device.GetFrameTime();

        if (device.IsResize())
        {
            driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
            camera.setAspectRatio(device.GetWidth() / device.GetHeight());
        }

        const float SPEED = 1.0f;

        Vec3 moveInput(0, 0, 0);

        if (Input::IsKeyDown(KEY_W))
            moveInput.z += SPEED; // Forward
        if (Input::IsKeyDown(KEY_S))
            moveInput.z -= SPEED; // Backward
        if (Input::IsKeyDown(KEY_A))
            moveInput.x -= SPEED; // Left
        if (Input::IsKeyDown(KEY_D))
            moveInput.x += SPEED; // Right
        if (Input::IsKeyDown(KEY_Q))
            moveInput.y -= SPEED; // Down
        if (Input::IsKeyDown(KEY_E))
            moveInput.y += SPEED; // Up

        cameraMove->setMoveInput(moveInput);

        if (Input::IsMouseDown(MouseButton::RIGHT))
        {
            Vec2 mouseDelta = Input::GetMouseDelta();
            cameraMove->setRotationInput(mouseDelta);
        }

        camera.update(dt);
        mainGgun.update(dt);
        // sliderel.update(dt);
        // trigger.update(dt);
 
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();
        const Vec3 cameraPos = camera.getPosition();
        Vec3 lightPos(-2.0f, 8.0f, -4.0f);

       // animationTime += dt * animationSpeed;
 

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);


        if (Input::IsKeyDown(KEY_P))
            gun.rotate(Vec3(0.0f, 1.0f, 0.0f), 0.1f);
        if (Input::IsKeyDown(KEY_O))
            gun.rotate(Vec3(0.0f, 1.0f, 0.0f), -0.1f);
       

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        sceneShader->SetUniform("useClipPlane", 0);
        sceneShader->SetUniform("diffuse", 0);

 
        
        
       
        mainGgun.render(sceneShader);
 
 
       


        if(Input::IsMouseDown(MouseButton::LEFT))
        {
            if (!animator->IsPlaying("shoot") && !animator->IsPlaying("reload") )
            {
                animator->PlayAnimationThen("shoot", "idle");
                shots++;
            }
        }

        if (shots > 10 && !animator->IsPlaying("reload"))
        {
            animator->PlayAnimationThen("reload", "idle");
            shots = 0;
        }

        if (Input::IsKeyDown(KEY_R) && !animator->IsPlaying("reload"))
        {
            animator->PlayAnimationThen("reload", "idle");
            shots = 0;
        }

      
    

        const Mat4 &mvp = proj * view;
        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);
        driver.SetCulling(CullMode::None);

        batch.Grid(10, 1.0f, true);

      //  meshRender->getMesh()->Debug(&batch);
  

        batch.Render();

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        batch.SetColor(255, 255, 255);

        font.Print(10, 10, "Fps %d", device.GetFPS());
        // gui.BeginFrame();

        // gui.BeginWindow("Scales", 20, 10, 260, 280);

        // float Y = 20;
        // gui.Label("Arms", 10, Y);
        // Y += 20;
        // gui.SliderFloat("", &armsScale, 0.0f, 1.0f, 20, Y, 200, 20);
        // Y += 20;
        // gui.Label("Main", 10, Y);
        // Y += 20;
        // gui.SliderFloat("", &mainScale, 0.0f, 1.0f, 20, Y, 200, 20);
        // Y += 20;
        // gui.Label("Slide", 10, Y);
        // Y += 20;
        // gui.SliderFloat("", &slideScale, 0.0f, 1.0f, 20, Y, 200, 20);
        // Y += 20;
        // gui.Label("Clip", 10, Y);
        // Y += 20;
        // gui.SliderFloat("", &clipScale, 0.0f, 1.0f, 20, Y, 200, 20);
        // Y += 20;
        // gui.Label("Frame", 10, Y);
        // Y += 20;
        // gui.SliderFloat("", &animationTime, 0.0f, 123.0f, 20, Y, 200, 20);
        

        // gui.EndWindow();

        // gui.EndFrame();




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
