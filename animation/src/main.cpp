

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
    GameObject *soldier;
    Joint3D *headNode;
    float angle = 0;

    AnimationLayer *torsoLayer;
    AnimationLayer *legsLayer;
    Mesh *soldierMesh;

    enum NPCState
    {
        NPC_IDLE,
        NPC_CHASE,
        NPC_ATTACK,
        NPC_RETREAT,
        NPC_SIDESTEP
    };
    NPCState npcState = NPC_IDLE;
    float npcStateTimer = 0.0f;
    int npcAttackCount = 0;
    float npcAttackCooldown = 0.0f;
    Vec3 npcSideStepDir = Vec3(1, 0, 0);

    GameObject *npcSinbad;
    Animator *npcAnimator;
    AnimationLayer *npcTorso;
    AnimationLayer *npcLegs;

    
public:
    void OnDebug(RenderBatch *batch) override 
    {

    //    terrain->debug(batch);

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

 

        renderPass(sceneShader, RenderType::Terrain);
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
        camera->setFOV(70.0f);
        camera->setNearPlane(0.1f);
        camera->setFarPlane(5000.0f);
        camera->setPosition(0.0f, 0.5f, 10.0f);

        TextureManager::Instance().SetLoadPath("assets/");
        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 10, 10);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        // GameObject *plane = createGameObject("Plane");
        // plane->addComponent<MeshRenderer>(mesh);
        // plane->setPosition(0.0f, 0.0f, 0.0f);

        mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));

        TextureManager::Instance().Add("sinbad/sinbad_body.tga", false);
        TextureManager::Instance().Add("sinbad/sinbad_clothes.tga", false);
        TextureManager::Instance().Add("sinbad/sinbad_sword.tga", false);

        Mesh *meshModel = MeshManager::Instance().Load("sinbad", "assets/sinbad/sinbad.h3d");

        if (meshModel)
        {

            // Bip01_Head = meshModel->FindBone("Bip01_Head");

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
                GameObject *sinbad = createGameObject("Sinbad");
                sinbad->addComponent<MeshRenderer>(meshModel);
                Animator *animator = sinbad->addComponent<Animator>();
                sinbad->setPosition(1, 0.5f, 0);
                sinbad->setScale(0.1f, 0.1f, 0.1f);

                torsoLayer = animator->AddLayer();
                torsoLayer->LoadAnimation("run", "assets/sinbad/sinbad_RunTop.anim");
                torsoLayer->LoadAnimation("idle", "assets/sinbad/sinbad_IdleTop.anim");
                torsoLayer->LoadAnimation("sliceHorizontal", "assets/sinbad/sinbad_SliceHorizontal.anim");
                torsoLayer->LoadAnimation("sliceVertical", "assets/sinbad/sinbad_SliceVertical.anim");

                torsoLayer->Play("idle", PlayMode::Once);

                legsLayer = animator->AddLayer();
                legsLayer->LoadAnimation("run", "assets/sinbad/sinbad_RunBase.anim");
                legsLayer->LoadAnimation("idle", "assets/sinbad/sinbad_IdleBase.anim");

                legsLayer->Play("idle", PlayMode::Loop);
            }

            {
                npcSinbad = createGameObject("NPCSinbad");
                npcSinbad->addComponent<MeshRenderer>(meshModel); // Usa o mesmo mesh
                npcAnimator = npcSinbad->addComponent<Animator>();
                npcSinbad->setPosition(-3, 0.5f, 0);
                npcSinbad->setScale(0.1f, 0.1f, 0.1f);

                npcTorso = npcAnimator->AddLayer();
                npcTorso->LoadAnimation("run", "assets/sinbad/sinbad_RunTop.anim");
                npcTorso->LoadAnimation("idle", "assets/sinbad/sinbad_IdleTop.anim");
                npcTorso->LoadAnimation("sliceH", "assets/sinbad/sinbad_SliceHorizontal.anim");
                npcTorso->LoadAnimation("sliceV", "assets/sinbad/sinbad_SliceVertical.anim");
                npcTorso->Play("idle", PlayMode::Loop);

                npcLegs = npcAnimator->AddLayer();
                npcLegs->LoadAnimation("run", "assets/sinbad/sinbad_RunBase.anim");
                npcLegs->LoadAnimation("idle", "assets/sinbad/sinbad_IdleBase.anim");
                npcLegs->Play("idle", PlayMode::Loop);
            }
        }

        TextureManager::Instance().SetFlipVerticalOnLoad(true);
        TextureManager::Instance().Add("ranger/face.jpg", true);
        TextureManager::Instance().Add("ranger/air.jpg", true);
        TextureManager::Instance().Add("ranger/body.jpg", true);
        TextureManager::Instance().Add("ranger/colt.jpg", true);

        soldierMesh = MeshManager::Instance().Load("ranger", "assets/ranger/ranger.h3d");
        if (soldierMesh)
        {

            soldierMesh->SetMaterialTexture(0, 0, TextureManager::Instance().Get("body"));
            soldierMesh->SetMaterialTexture(1, 0, TextureManager::Instance().Get("face"));
            soldierMesh->SetMaterialTexture(2, 0, TextureManager::Instance().Get("air"));

            soldier = createGameObject("Soldier");
            soldier->setPosition(-4, 0, 0);
            soldier->setScale(0.01f);
            soldier->addComponent<MeshRenderer>(soldierMesh);

            Animator *soldierAnimator = soldier->addComponent<Animator>();
            soldierAnimator->awake();

            for (u32 i = 0; i < soldier->getJointCount(); i++)
            {
                GameObject *cube = createGameObject("Cube", soldier->getJoint(i));
                cube->addComponent<MeshRenderer>(mesh);
                cube->setScale(20.5f);
            }

            headNode = soldier->findJoint("Bip01_Head");
            // headNode =  soldier->findJoint("Bip01_Spine3");

            headNode->SetControllable(true);

            AnimationLayer *torsoLayer = soldierAnimator->AddLayer();
            torsoLayer->LoadAnimation("topRun", "assets/ranger/ranger_Gun_Shooting_Pose.anim");
            torsoLayer->Play("topRun", PlayMode::Loop);

            AnimationLayer *legsLayer = soldierAnimator->AddLayer();
            legsLayer->LoadAnimation("legsRun", "assets/ranger/ranger_Walk_Legs_Only.anim");
            legsLayer->Play("legsRun", PlayMode::Loop);
        }

        Pixmap tiles;
        tiles.Load("assets/tiles.png");

        TiledTerrain *terrain = createTiledTerrain("Terrain",8,5,8,16,1);
        terrain->LoadTilemap(&tiles);
        terrain->GetMaterial()->SetTexture(0, TextureManager::Instance().Add("circleTextures64.jpg"));
 

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

        if (Input::IsKeyDown(KEY_ONE))
            torsoLayer->Play("run", PlayMode::Once, 0.5f);

        if (Input::IsKeyDown(KEY_TWO))
            torsoLayer->Play("idle", PlayMode::Once, 0.01f);
        if (Input::IsKeyDown(KEY_THREE))
            torsoLayer->Play("sliceHorizontal", PlayMode::Once, 0.5f);
        if (Input::IsKeyDown(KEY_FOUR))
            torsoLayer->Play("sliceVertical", PlayMode::Once, 0.5f);

        if (Input::IsKeyDown(KEY_FIVE))
            legsLayer->Play("run", PlayMode::Backward, 0.5f);

        if (Input::IsKeyDown(KEY_SIX))
            legsLayer->Play("run", PlayMode::Once, 0.5f);

        if (Input::IsKeyDown(KEY_SEVEN))
            legsLayer->Play("idle", PlayMode::Once, 0.5f);

        cameraMove->setMoveInput(moveInput);

        camera->update(dt  * 25.0f);
        // animator->Update(dt);
        //  soldierAnimator->Update(dt);

        angle += 10.0f * dt;
        headNode->rotateX(sin(angle) * 0.1f);
        // soldier->rotateY(angle);

        Vec3 npcPos = npcSinbad->getPosition();
        Vec3 camPos = camera->getPosition();
        Vec3 dirToCamera = camPos - npcPos;
        dirToCamera.y = 0; // Ignora altura
        float distToCamera = dirToCamera.length();
        dirToCamera.normalize();

        const float ATTACK_RANGE = 1.0f;
        // const float RETREAT_RANGE = 2.0f;
        const float MOVE_SPEED = 2.0f;
        const float ROTATE_SPEED = 3.0f;

        npcStateTimer += dt;
        if (npcAttackCooldown > 0)
            npcAttackCooldown -= dt;

        // Rotaciona NPC para olhar para câmera
        Vec3 forward = npcSinbad->getBackward();

        forward.y = 0;
        forward.normalize();
        float angle = atan2(dirToCamera.x, dirToCamera.z) - atan2(forward.x, forward.z);

        npcSinbad->rotateY(angle * ROTATE_SPEED * dt);

        // ===== MÁQUINA DE ESTADOS =====
        switch (npcState)
        {
        case NPC_IDLE:
        {
            npcTorso->Play("idle", PlayMode::Loop, 0.3f);
            npcLegs->Play("idle", PlayMode::Loop, 0.3f);

            if (distToCamera < 10.0f && npcStateTimer > 1.0f)
            {
                npcState = NPC_CHASE;
                npcStateTimer = 0.0f;
            }
            break;
        }

        case NPC_CHASE:
        {
            npcTorso->Play("run", PlayMode::Loop, 0.3f);
            npcLegs->Play("run", PlayMode::Loop, 0.3f);

            // Move em direção à câmera
            npcPos += dirToCamera * MOVE_SPEED * dt;
            npcSinbad->setPosition(npcPos.x, 0.5f, npcPos.z);

            if (distToCamera <= ATTACK_RANGE)
            {
                npcState = NPC_ATTACK;
                npcStateTimer = 0.0f;
                npcAttackCount = 0;
            }
            else if (distToCamera > 15.0f)
            {
                npcState = NPC_IDLE;
                npcStateTimer = 0.0f;
            }
            break;
        }

        case NPC_ATTACK:
        {
            npcLegs->Play("idle", PlayMode::Loop, 0.2f);

            // Ataca alternando entre horizontal e vertical
            if (npcAttackCooldown <= 0.0f)
            {
                if (npcAttackCount % 2 == 0)
                    npcTorso->Play("sliceH", PlayMode::Once, 0.2f);
                else
                    npcTorso->Play("sliceV", PlayMode::Once, 0.2f);

                npcAttackCount++;
                npcAttackCooldown = 0.8f;

                // Após 2 ataques, escolhe ação
                if (npcAttackCount >= 2)
                {
                    int choice = rand() % 3;
                    if (choice == 0)
                    {
                        // Recua
                        npcState = NPC_RETREAT;
                        npcStateTimer = 0.0f;
                    }
                    else if (choice == 1)
                    {
                        // Passo lateral
                        npcState = NPC_SIDESTEP;
                        npcStateTimer = 0.0f;
                        npcSideStepDir = (rand() % 2 == 0) ? Vec3(dirToCamera.z, 0, -dirToCamera.x) : // Esquerda
                                             Vec3(-dirToCamera.z, 0, dirToCamera.x);                  // Direita
                    }
                    else
                    {
                        // Continua atacando
                        npcAttackCount = 0;
                    }
                }
            }

            // Se câmera fugiu, persegue
            if (distToCamera > ATTACK_RANGE + 1.0f)
            {
                npcState = NPC_CHASE;
                npcStateTimer = 0.0f;
                npcAttackCount = 0;
            }
            break;
        }

        case NPC_RETREAT:
        {
            // Toca animação de correr ao contrário (NOVO!)
            npcTorso->Play("run", PlayMode::Backward, 0.3f);
            npcLegs->Play("run", PlayMode::Backward, 0.3f);

            // Move para trás
            npcPos -= dirToCamera * MOVE_SPEED * 0.7f * dt;
            npcSinbad->setPosition(npcPos.x, 0.5f, npcPos.z);

            if (npcStateTimer > 1.0f) // Recua por 1 segundo
            {
                npcState = NPC_CHASE;
                npcStateTimer = 0.0f;
                npcAttackCount = 0;
            }
            break;
        }

        case NPC_SIDESTEP:
        {
            npcTorso->Play("run", PlayMode::Loop, 0.2f);
            npcLegs->Play("run", PlayMode::Loop, 0.2f);

            // Move lateralmente
            npcPos += npcSideStepDir * MOVE_SPEED * 1.2f * dt;
            npcSinbad->setPosition(npcPos.x, 0.5f, npcPos.z);

            if (npcStateTimer > 0.6f) // Passo lateral rápido
            {
                npcState = NPC_ATTACK;
                npcStateTimer = 0.0f;
                npcAttackCount = 0;
                npcAttackCooldown = 0.3f;
            }
            break;
        }
        }
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

        // batch.Grid(10, 1.0f, true);

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
