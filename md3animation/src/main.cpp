

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


    MeshM3D *meshPlayerLower = MeshManager::Instance().LoadM3D("PlayerLower","assets/md3/orion/lower.md3", 1.0f);

    if (!meshPlayerLower)
    {
        LogError("Failed to load player");
        return false;
    }

    MeshM3D *meshPlayerUpper = MeshManager::Instance().LoadM3D("PlayerUpper","assets/md3/orion/upper.md3", 1.0f);

    if (!meshPlayerUpper)
    {
        LogError("Failed to load player");
        return false;
    }

    MeshM3D *meshPlayerHead = MeshManager::Instance().LoadM3D("PlayerHead","assets/md3/orion/head.md3", 0.4f);

    if (!meshPlayerHead)
    {
        LogError("Failed to load player");
        return false;
    }

// upper tag_head (0)
// upper tag_fun (1)
// upper tag_weapon (2)
// upper tag_back (3)
// upper tag_primary (4)
// upper tag_second (5)
// upper tag_funback (6)
// upper tag_flag (7)
// upper tag_torso (8)

    GameObject player("Player");
    GameObject playerLower("PlayerLegs");
    playerLower.setParent(&player);
    playerLower.rotateDeg(Vec3(0.0f, 0.0f, 1.0f), 90.0f);
    playerLower.rotateDeg(Vec3(0.0f, 1.0f, 0.0f), 90.0f);
    playerLower.addComponent<MeshM3DRenderer>(meshPlayerLower);
    VertexAnimator *lowerAnimation = playerLower.addComponent<VertexAnimator>();
    lowerAnimation->AddAnimation("WALK", 385, 385 + 20, 11.0f, true);
    lowerAnimation->PlayAnimation("WALK");
    
    GameObject playerUpper("PlayerTorso");
    playerUpper.setParent(playerLower.getJoint(1));
    playerUpper.addComponent<MeshM3DRenderer>(meshPlayerUpper);
    VertexAnimator *upperAnimation = playerUpper.addComponent<VertexAnimator>();
    upperAnimation->AddAnimation("STAND_PISTOL", 458, 458, 15, true);
    upperAnimation->AddAnimation("POINT", 653, 653 + 38, 15.0f, true);
    upperAnimation->PlayAnimation("STAND_PISTOL");

    GameObject playerHead("PlayerHead");
    playerHead.setParent(playerUpper.getJoint(1));
    playerHead.addComponent<MeshM3DRenderer>(meshPlayerHead);

    


    MeshM3D *berreta = MeshManager::Instance().LoadM3D("Berreta","assets/md3/beretta/beretta_hold.md3", 100.0f);

    if (!berreta)
    {
        LogError("Failed to load berreta");
        return false;
    }

    MeshM3D *cigar = MeshManager::Instance().LoadM3D("Cigar","assets/md3/cigar/cigar_r_3_1.md3", 0.5f);

    MeshM3D *specs[5];

    specs[0] = MeshManager::Instance().LoadM3D("Main","assets/md3/beretta/beretta.md3", 30.0f);
    specs[1] = MeshManager::Instance().LoadM3D("Slide","assets/md3/beretta/beretta_view_slide.md3", 25.0f);
    specs[2] = MeshManager::Instance().LoadM3D("Clip","assets/md3/beretta/beretta_view_clip.md3", 6.0f);
    specs[3] = MeshManager::Instance().LoadM3D("Safety","assets/md3/beretta/beretta_view_safety.md3", 1.0f);
    specs[4] = MeshManager::Instance().LoadM3D("Sliderel","assets/md3/beretta/beretta_view_sliderel.md3", 1.0f);

    

    

 

    TextureManager::Instance().SetLoadPath("assets/");
    TextureManager::Instance().SetFlipVerticalOnLoad(false);

    {
        Material *material = meshPlayerHead->GetMaterial(0);
        material->SetTexture(0, TextureManager::Instance().Add("md3/orion/head_free_b.png"));
        
    }
    {
        Material *material = meshPlayerUpper->GetMaterial(0);
        material->SetTexture(0, TextureManager::Instance().Add("md3/orion/upper_free_b.png"));
        
    }
    {
        Material *material = meshPlayerLower->GetMaterial(0);
        material->SetTexture(0, TextureManager::Instance().Add("md3/orion/lower_free_b.png"));
    }
    {
        Material *material = cigar->GetMaterial(0);
       material->SetTexture(0, TextureManager::Instance().Add("md3/cigar/cigar1.png"));
    }
    
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
    
    GameObject playerGunHold("playergunhold");
    playerGunHold.setParent(playerUpper.getJoint(2));
  //  playerGunHold.setScale(0.010f);
    
    
    GameObject playrgun("playergun");
    playrgun.setParent(&playerGunHold);
    playrgun.addComponent<MeshM3DRenderer>(specs[0]);
    playrgun.setScale(0.0115f);
    playrgun.setPosition(-0.055f, 0.0f, 0.025f);

    GameObject playerCigarHold("playergunhold");
    playerCigarHold.setParent(playerUpper.getJoint(1));

    GameObject playCigar("playercigar");
    playCigar.setParent(&playerCigarHold);
    playCigar.addComponent<MeshM3DRenderer>(cigar);
    playCigar.setPosition(-0.05f, 0.0f, -0.025f);
    playCigar.setScale(0.6f);
    //playCigar.rotateDeg(Vec3(0.0f, 0.0f, 1.0f), 90.0f);
   // playCigar.rotateDeg(Vec3(0.0f, 1.0f, 0.0f), 90.0f);

    //playCigar.setScale(0.0115f);
    //playCigar.setPosition(-0.055f, 0.0f, 0.025f);
    
// INFO: [18:33:18]: Tag: tag_head (0)
// INFO: [18:33:18]: Tag: tag_fun (1)
// INFO: [18:33:18]: Tag: tag_weapon (2)
// INFO: [18:33:18]: Tag: tag_back (3)
// INFO: [18:33:18]: Tag: tag_primary (4)
// INFO: [18:33:18]: Tag: tag_second (5)
// INFO: [18:33:18]: Tag: tag_funback (6)
// INFO: [18:33:18]: Tag: tag_flag (7)
// INFO: [18:33:18]: Tag: tag_torso (8)

    GameObject gun("gun");
    gun.setParent(&camera);
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

    GameObject gunSlide("gunSlide");
    gunSlide.setParent(gun.getJoint(5));
    gunSlide.addComponent<MeshM3DRenderer>(specs[1]);

    GameObject gunClip("gunClip");
    gunClip.setParent(gun.getJoint(2));
    gunClip.addComponent<MeshM3DRenderer>(specs[2]);

    GameObject gunSafety("gunSafety");
    gunSafety.setParent(gun.getJoint(4));
    gunSafety.addComponent<MeshM3DRenderer>(specs[3]);





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
    




    gun.scale(Vec3(0.01f));
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

 
        player.update(dt);
   
        
        
      
  
        {
        const Mat4 &model = gun.getWorldTransform();
        sceneShader->SetUniformMat4("model", model.m);
        gun.render(sceneShader);
        }
 
       


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
