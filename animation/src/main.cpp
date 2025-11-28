

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;
    Camera *camera;

    FreeCameraComponent *cameraMove;

    float mouseSensitivity{0.8f};
    float roll = 3.0;
 
    Animator *animator;
    GameObject *soldier ;
 
Mesh *soldierMesh;
public:
    void OnDebug(RenderBatch *batch)  override
    {

       
    };
    void OnRender() override
    {

        Device &device = Device::Instance();
        Driver &driver = Driver::Instance();
        Vec3 lightPos(-2.0f, 8.0f, -4.0f);

        driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
        camera->setAspectRatio(device.GetWidth() / device.GetHeight());
        SetCamera(camera);
        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();
        const Vec3 cameraPos = camera->getPosition();

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        renderPass(sceneShader, RenderType::Solid);

        // Mat4  model = Mat4::Translation(Vec3(-2.0f, 0.5f, 0.0f)) * Mat4::Scale(Vec3(0.1f));
        // sceneShader->SetUniformMat4("model", model.m);
        // driver.DrawMesh(meshModel);
 

        // renderAll(sceneShader);
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");

        if (!sceneShader)
            return false;

        camera = createCamera("CameraFree");
        cameraMove = camera->addComponent<FreeCameraComponent>();
        cameraMove->setMoveSpeed(15.0f);
        cameraMove->setMouseSensitivity(0.15f);
        cameraMove->setSprintMultiplier(3.0f);
        camera->setAspectRatio((float)screenWidth / (float)screenHeight);
        camera->setFOV(45.0f);
        camera->setNearPlane(0.1f);
        camera->setFarPlane(1000.0f);
        camera->setPosition(0.0f, 0.5f, 10.0f);

        TextureManager::Instance().SetLoadPath("assets/");
        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 10, 10);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        GameObject *plane = createGameObject("Plane");
        plane->addComponent<MeshRenderer>(mesh);
        plane->setPosition(0.0f, -2.0f, 0.0f);

        mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));
        Vec3 cubePositions[] = {
            Vec3(0.0f, 0.5f, 5.0f),
            Vec3(3.0f, 0.5f, -3.0f),
            Vec3(-3.0f, 0.5f, -3.0f),
            Vec3(5.0f, 0.5f, 2.0f),
            Vec3(-5.0f, 0.5f, 2.0f),
            Vec3(2.0f, 0.5f, -6.0f),
            Vec3(-2.0f, 0.5f, -6.0f),
        };

        for (int i = 0; i < 7; i++)
        {
          //  GameObject *cube = createGameObject("Cube");
          //  cube->addComponent<MeshRenderer>(mesh);
          //  cube->setPosition(cubePositions[i]);

            //   Rotator* rotator = cube->addComponent<Rotator>();
            //    rotator->setRotationSpeed(Vec3(0, 90, 20));  // 90°/s on Y axis
        }

        TextureManager::Instance().Add("sinbad/sinbad_body.tga", false);
        TextureManager::Instance().Add("sinbad/sinbad_clothes.tga", false);
        TextureManager::Instance().Add("sinbad/sinbad_sword.tga", false);

        Mesh * meshModel = MeshManager::Instance().Load("sinbad", "assets/sinbad/sinbad.h3d");

        if (meshModel)
        {


           

            //Bip01_Head = meshModel->FindBone("Bip01_Head");

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

            GameObject *sinbad = createGameObject("Sinbad");
            sinbad->addComponent<MeshRenderer>(meshModel);
            Animator* animator = sinbad->addComponent<Animator>();
            sinbad->setPosition(1, 0, 0);
            sinbad->setScale(0.5f, 0.5f, 0.5f);

            
            AnimationLayer *torsoLayer = animator->AddLayer();
            torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_RunTop.anim");
            torsoLayer->Play("topRun", PlayMode::Loop);
    
            AnimationLayer *legsLayer = animator->AddLayer();
            legsLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");
            legsLayer->Play("legsRun", PlayMode::Loop);
        }

        TextureManager::Instance().SetFlipVerticalOnLoad(true);
        TextureManager::Instance().Add("ranger/face.jpg", true);
        TextureManager::Instance().Add("ranger/air.jpg", true);
        TextureManager::Instance().Add("ranger/body.jpg", true);
        TextureManager::Instance().Add("ranger/colt.jpg", true);


      

        

        soldierMesh = MeshManager::Instance().Load("ranger", "assets/ranger/ranger.h3d");
        if (soldierMesh)
        {
            
            soldierMesh->SetMaterialTexture(0,0, TextureManager::Instance().Get("body"));
            soldierMesh->SetMaterialTexture(1,0, TextureManager::Instance().Get("face"));
            soldierMesh->SetMaterialTexture(2,0, TextureManager::Instance().Get("air"));
 


            // GameObject *soldierRoot = createGameObject("SoldierRoot");
            //  soldierRoot->setScale(0.01f);
            //  soldierRoot->setPosition(-2, 0, 0);
//             soldierMesh->SetBoneParent("Bip01_Pelvis", soldierRoot);
            

            // Node3D *rootNode =  soldierMesh->FindBone("Bip01_Pelvis");
 

             Bone *headNode =  soldierMesh->FindBone("Bip01_R_Hand");
        
            //Bip01_R_Hand
            //Bip01_Head


             
             
             
             soldier = createGameObject("Soldier");
         //    soldier->setPosition(-200, 0, 0);
          //   soldier->setScale(0.01f);
             soldier->addComponent<MeshRenderer>(soldierMesh);


          //  Node3D *headNode =  soldier->getJoint("Bip01_Head");

            GameObject *cube = createGameObject("Cube",headNode);
            cube->addComponent<MeshRenderer>(mesh);
            cube->setScale(10.5f);

     
      

             
             
            //   rootNode->setParent(soldier);
             
            //  GameObject *soldierRoot = createGameObject("SoldierRoot",soldier);
            //   soldierRoot->setScale(0.01f);
            //  soldierMesh->SetBoneParent("Bip01_Pelvis", soldierRoot);

            //  soldierRoot->setParent(soldier);

          //rootNode->setParent(soldier);


             
            Animator *soldierAnimator = soldier->addComponent<Animator>();
  
            
           
            AnimationLayer *torsoLayer  = soldierAnimator->AddLayer();
            torsoLayer->LoadAnimation("topRun", "assets/ranger/ranger_Gun_Shooting_Pose.anim");
            torsoLayer->Play("topRun", PlayMode::Loop);
    
            AnimationLayer *legsLayer = soldierAnimator->AddLayer();
            legsLayer->LoadAnimation("legsRun", "assets/ranger/ranger_Walk_Legs_Only.anim");
            legsLayer->Play("legsRun", PlayMode::Loop); 
        }

  

        return true;
    }
    void OnDestroy() override
    {
        
    }
    void OnUpdate(float dt) override
    {
        const float SPEED = 90.0f;

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
       // animator->Update(dt);
      //  soldierAnimator->Update(dt);

    }
    void OnResize(u32 w, u32 h) override
    {
        camera->setAspectRatio((float)w / (float)h);
    }
    Camera *getCamera() { return camera; }
    FreeCameraComponent *getCameraControl() { return cameraMove; };
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

    GUI gui;
    gui.Init(&batch, &font);

    MainScene scene;
    if (!scene.Init())
    {
        device.Close();
        return 1;
    }
    scene.OnResize(device.GetWidth(), device.GetHeight());

    while (device.Run())
    {

        float dt = device.GetFrameTime();

        if (device.IsResize())
        {
            driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
            scene.OnResize(device.GetWidth(), device.GetHeight());
        }

        scene.Update(dt);

        const Mat4 &view = scene.getViewMatrix();
        const Mat4 &proj = scene.getProjectionMatrix();
        const Mat4 &mvp = proj * view;
        const Vec3 &cameraPos = scene.getCamera()->getPosition();

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        scene.Render();

        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);


        batch.Grid(10, 1.0f, true);

        scene.Debug(&batch);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

        // Stats window
        gui.BeginWindow("Stats", screenWidth - 260, 10, 260, 170);
        gui.Text(10, 10, "FPS %d Delta: %.2f ms", device.GetFPS(), dt);

        int drawCalls = driver.GetCountDrawCall();
        int meshs = driver.GetCountMesh();
        int meshBuffers = driver.GetCountMeshBuffer();
        int vertices = driver.GetCountVertex();
        int triangles = driver.GetCountTriangle();
        int textures = driver.GetCountTextures();
        int shaders = driver.GetCountPrograms();

        gui.Text(10, 30, "Draw Calls: %d", drawCalls);
        gui.Text(10, 50, "Mesh: %d    Buffers: %d", meshs, meshBuffers);

        gui.Text(10, 70, "Vertices: %d Triangles: %d", vertices, triangles);
        gui.Text(10, 90, "Textures: %d Shaders: %d", textures, shaders);

        gui.Text(10, 110, "Camera: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);

        gui.EndWindow();

        gui.EndFrame();
        batch.Render();

        if (Input::IsMouseDown(MouseButton::LEFT) && !gui.IsFocused())
        {
            Vec2 mouseDelta = Input::GetMouseDelta();
            scene.getCameraControl()->setRotationInput(mouseDelta);
        }

        batch.Render();

        device.Flip();
    }

    scene.Release();
    font.Release();
    batch.Release();
    device.Close();

    return 0;
}
