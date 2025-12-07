

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

enum class AIState
{
    Stop,
    Pursue,
    Evade,
    Attack
};

class MainScene : public Scene
{
    Shader *sceneShader;
    Camera *camera;

    FreeCameraComponent *cameraMove;

    float mouseSensitivity{0.8f};

    GameObject *leftSword;
    GameObject *rightSword;
    SteeringComponent *steering = nullptr;
    GameObject *sinbad = nullptr;
    GameObject *dummy = nullptr;
    AIState aiState = AIState::Stop;

    AnimationLayer *torsoLayer = nullptr;
    AnimationLayer *legsLayer = nullptr;

    Vec3 cameraPrevPos{0, 0, 0};
    Vec3 cameraVelocity{0, 0, 0};

    // Radii em unidades de mundo
    float aiPursueRadius = 15.0f; // começa a perseguir
    float aiEvadeRadius = 3.0f;   // demasiado perto → fuga
    float aiAttackRadius = 1.5f;  // muito perto → ataque

    // Opcional: cooldown simples para ataque
    float aiAttackCooldown = 0.0f;     // seg
    float aiAttackCooldownTime = 2.0f; // seg entre ataques

    float distanceToTarget = 0.0f;

    float angleToTarget = 0.0f;

public:
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

        cameraPrevPos = camera->getPosition();

        TextureManager::Instance().SetLoadPath("assets/");
        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 10, 10);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        GameObject *plane = createGameObject("Plane");
        plane->addComponent<MeshRenderer>(mesh);

        mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));

        TextureManager::Instance().Add("sinbad/sinbad_body.tga", true);
        TextureManager::Instance().Add("sinbad/sinbad_clothes.tga", true);
        TextureManager::Instance().Add("sinbad/sinbad_sword.tga", true);

        Mesh *cube = MeshManager::Instance().CreateCube("cube", 1.0f);

        dummy = createGameObject("Dummy");
        dummy->addComponent<MeshRenderer>(cube);

        Mesh *meshSword = MeshManager::Instance().Load("sinbad_sword", "assets/sinbad/sword.h3d");

        if (meshSword)
        {
            Material *material = meshSword->GetMaterial(0);
            material->SetTexture(0, TextureManager::Instance().Get("sinbad_sword"));
        }

        Mesh *meshModel = MeshManager::Instance().Load("sinbad", "assets/sinbad/sinbad.h3d");

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

            {
                sinbad = createGameObject("Sinbad");
                steering = sinbad->addComponent<SteeringComponent>();

                GameObject *body = createGameObject("Body", sinbad);
                body->rotateYDeg(180.0f);
                body->addComponent<MeshRenderer>(meshModel);

                Animator *animator = body->addComponent<Animator>();
                sinbad->setPosition(0.0f, 0.5f, 0);
                sinbad->setScale(0.1f, 0.1f, 0.1f);

                Joint3D *leftHand = body->getJoint("Hand.L");
                Joint3D *rightHand = body->getJoint("Hand.R");

                leftSword = createGameObject("LeftSword");
                leftSword->addComponent<MeshRenderer>(meshSword);
                leftSword->setPosition(0.8f, 0.8f, -0.5f);
                leftSword->setParent(leftHand);

                GameObject *leftSwordEnd = createGameObject("LeftSwordEnd");
                leftSwordEnd->setPosition(0.0f, 0.5f, 5.5f);
                leftSwordEnd->setParent(leftSword);

                rightSword = createGameObject("RightSword");
                rightSword->addComponent<MeshRenderer>(meshSword);
                rightSword->setPosition(-0.8f, 0.8f, 0.0f);
                rightSword->setParent(rightHand);

                GameObject *rightSwordEnd = createGameObject("RightSwordEnd");
                rightSwordEnd->setPosition(0.0f, 0.5f, 5.5f);
                rightSwordEnd->setParent(rightSword);

                rightSword->setEulerAnglesDeg(Vec3(-5.171500, -73.914894, -162.641190));
                leftSword->setEulerAnglesDeg(Vec3(-24.686144, 110.653992, 142.517471));

                AnimationLayer *handsLayer = animator->AddLayer();
                handsLayer->LoadAnimation("topIdle", "assets/sinbad/sinbad_HandsClosed.anim");

                handsLayer->Play("topIdle", PlayMode::Loop);

                torsoLayer = animator->AddLayer();
                torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_RunTop.anim");
                torsoLayer->LoadAnimation("topIdle", "assets/sinbad/sinbad_IdleTop.anim");
                torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_Dance.anim");
                Animation *conf = torsoLayer->LoadAnimation("topSwing", "assets/sinbad/sinbad_DrawSwords.anim");
                conf->SetTicksPerSecond(0.3);

                legsLayer = animator->AddLayer();
                legsLayer->LoadAnimation("legsIdle", "assets/sinbad/sinbad_IdleBase.anim");
                legsLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");
                legsLayer->Play("legsRun", PlayMode::Loop);

                conf = torsoLayer->LoadAnimation("sliceVertical", "assets/sinbad/sinbad_SliceVertical.anim");
                conf->SetTicksPerSecond(0.4);
                conf = torsoLayer->LoadAnimation("sliceHorizontal", "assets/sinbad/sinbad_SliceHorizontal.anim");
                conf->SetTicksPerSecond(0.4);

                torsoLayer->Play("topIdle", PlayMode::Loop);
            }
        }

        return true;
    }
    void OnDestroy() override
    {
    }
    void OnUpdate(float dt) override
    {
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

        Vec3 currCamPos = camera->getPosition();
        if (dt > 0.0f)
            cameraVelocity = (currCamPos - cameraPrevPos) / dt;
        else
            cameraVelocity = Vec3(0);
        cameraPrevPos = currCamPos;

      
        Vec3 sinbadPos = sinbad->getPosition();
        Vec3 toCam = currCamPos - sinbadPos;
        distanceToTarget = toCam.length();
        angleToTarget = 0;

       
        if (aiAttackCooldown > 0.0f)
            aiAttackCooldown -= dt;

     
    switch (aiState)
    {
        case AIState::Stop:
        {
            if (distanceToTarget < aiAttackRadius && aiAttackCooldown <= 0.0f)
                aiState = AIState::Attack;
            else if (distanceToTarget < aiPursueRadius)
                aiState = AIState::Pursue;
            break;
        }

        case AIState::Pursue:
        {
            if (distanceToTarget < aiAttackRadius && aiAttackCooldown <= 0.0f)
                aiState = AIState::Attack;
            else if (distanceToTarget > aiPursueRadius * 1.5f)
                aiState = AIState::Stop;
            break;
        }

        case AIState::Evade:
        {
            if (distanceToTarget > aiEvadeRadius * 1.5f)
            {
                if (distanceToTarget < aiPursueRadius)
                    aiState = AIState::Pursue;
                else
                    aiState = AIState::Stop;
            }
            break;
        }

        case AIState::Attack:
        {
            // Quando terminar o ataque (cooldown), volta a perseguir ou a vaguear
            if (aiAttackCooldown <= 0.0f)
            {
                 if (distanceToTarget < aiPursueRadius)
                     aiState = AIState::Pursue;
                 else
                     aiState = AIState::Stop;
            }
            break;
        }
    }

        Vec3 force(0.0f);

    switch (aiState)
    {
        case AIState::Stop:
        {
               force = Vec3(0.0f);
            
            if (torsoLayer->GetCurrentAnimation() != "topIdle") 
                torsoLayer->Play("topIdle", PlayMode::Loop);
            if (legsLayer->GetCurrentAnimation() != "legsIdle")  
                legsLayer->Play("legsIdle", PlayMode::Loop);
            break;
        }

        case AIState::Pursue:
        {
         
            force = steering->pursue(currCamPos, cameraVelocity, 1.0f);

            if (torsoLayer->GetCurrentAnimation() != "topRun")
                 torsoLayer->Play("topRun", PlayMode::Loop);
            
            if (legsLayer->GetCurrentAnimation() != "legsRun"   )  
                legsLayer->Play("legsRun", PlayMode::Loop);
         
            break;
        }

        case AIState::Evade:
        {
            // Se já tens evade, usa-o; senão, flee:
            force = steering->flee(currCamPos, 1.0f);

            // if (torsoLayer) torsoLayer->Play("topRun", PlayMode::Loop);
            // if (legsLayer)  legsLayer->Play("legsRun", PlayMode::Loop);
            break;
        }

        case AIState::Attack:
        {
           
            force =Vec3::Zero;

           
                steering->stop();
        
            // Olhar para a câmara
            {
                Vec3 dir = currCamPos - sinbadPos;
                dir.y = 0.0f;
                if (dir.lengthSquared() > 0.0001f)
                {
                    dir = dir.normalized();
                    Vec3 lookTarget = sinbadPos + dir;
                    sinbad->lookAt(lookTarget, TransformSpace::World, Vec3(0,1,0));
                }
            }

           
            if (torsoLayer->GetCurrentAnimation() != "sliceVertical")
                torsoLayer->Play("sliceVertical", PlayMode::Loop);
           
            if (legsLayer->GetCurrentAnimation() != "legsIdle")
                legsLayer->Play("legsIdle", PlayMode::Loop);
            

            // Começa cooldown para não spammar ataques
            if (aiAttackCooldown <= 0.0f)
                aiAttackCooldown = aiAttackCooldownTime;

                
                break;
            }
        }
        
        steering->addForce(force);

    }
    void OnResize(u32 w, u32 h) override
    {
        camera->setAspectRatio((float)w / (float)h);
    }
    void OnGui(GUI &gui)
    {

        bool open = true;

        const Color Position(230, 60, 60, 255);
        const Color Rotation(60, 220, 60, 255);
        const Color Scale(80, 120, 255, 255);

        if (!gui.BeginWindow("Node Inspector", 300, 10, 260, 180, &open))
        {
            gui.EndWindow();
            return;
        }

        gui.Text(10, 5, "Node: %s", sinbad->getName().c_str());

        float y = 20.0f;
        std::string steeringName = "None";
        switch (aiState)
        {
            case AIState::Stop:
                steeringName = "Stop";
                break;
            case AIState::Pursue:
                steeringName = "Pursue";
                break;
            case AIState::Evade:
                steeringName = "Evade";
                break;
            case AIState::Attack:
                steeringName = "Attack";
                break;
        }

        gui.Text(10, y, "Steering: %s", steeringName.c_str());
        y += 15.0f;

        gui.Text(10, y, "Distance: %.2f, Angle: %.2f", distanceToTarget, angleToTarget);
        y += 15.0f;

        gui.EndWindow();
    }

    void OnDebug(RenderBatch *batch) override
    {

        Vec3 pos = sinbad->getPosition();
        Vec3 forward = sinbad->getForward();
        forward.y = 0;
        forward = forward.normalized();

        Vec3 circleCenter = pos + forward * 5.0f;
        batch->SetColor(255, 255, 0);
        batch->CircleXZ(circleCenter, 1.0f, 12);

        batch->SetColor(255, 0, 0);
        batch->Line3D(pos, circleCenter + forward * 1.0f);
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
        scene.OnGui(gui);

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
