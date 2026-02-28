

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

// Configuração CSM
const int CASCADE_COUNT = 4;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

class MainScene : public Scene
{
    Shader *depthShader;
    Shader *debugShader;
    Shader *shader;
    Camera *camera;
    Shader *skyShader;
    FreeCameraComponent *cameraMove;

    float mouseSensitivity{0.8f};
    float roll = 3.0;

    float angle = 0;

    // disk
    float debugBaseBias = 0.0002f;
    float debugSlopeBias = 0.0001f;
    float debugDiskRadius = 0.68f;

    // float debugBaseBias = 0.0004f;
    // float debugSlopeBias = 0.00015f;
    // float debugDiskRadius = 0.68f;

    float biasStep = 0.0005f;
    float radiusStep = 0.001f;
    GameObject *leftSword;
    GameObject *rightSword;

    GameObject *plane;
    ShadowMapManager *shadowManager;
    Shader *trailShader;
    AnimationLayer *animation;
    RibbonTrail *ribbon;
    float time;

public:
    void OnDebug(RenderBatch *batch) override {

    };
    void OnRender() override
    {

        Device &device = Device::Instance();
        Driver &driver = Driver::Instance();
        Vec3 lightPos(-20, 24, 24);

        Vec3 lightDir = lightPos.normalized();

        driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
        camera->setAspectRatio(device.GetWidth() / device.GetHeight());
        SetCamera(camera);
        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();
        const Vec3 cameraPos = camera->getPosition();

        float nearPlane = 0.1f;
        float farPlane = 800.0f;

        shadowManager->Calculate(view, proj, nearPlane, farPlane, lightDir);

        depthShader->Bind();
        shadowManager->BeginDepthPass();
        for (int i = 0; i < shadowManager->GetCascadeCount(); ++i)
        {
            shadowManager->BindCascade(i);
            depthShader->SetUniformMat4("lightSpaceMatrix", shadowManager->GetLightSpaceMatrix(i).m);
            renderPass(depthShader, RenderType::Solid);
        }
        shadowManager->EndDepthPass();
        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
        // Renderizar cena com sombras
        shader->Bind();
        shader->SetUniform("diffuseTexture", 0);
        shader->SetUniform("cascadeCount", CASCADE_COUNT);
        shader->SetUniform("showCascades", 0);
        shader->SetUniformMat4("projection", proj.m);
        shader->SetUniformMat4("view", view.m);
        shader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        shader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        shader->SetUniform("farPlane", farPlane);
        shader->SetUniform("shadowMapSize", SHADOW_WIDTH, SHADOW_HEIGHT);
        shader->SetUniform("debugBaseBias", debugBaseBias);
        shader->SetUniform("debugSlopeBias", debugSlopeBias);
        shader->SetUniform("debugDiskRadius", debugDiskRadius);
        shadowManager->SetShaderUniforms(shader);
        shadowManager->BindShadowMapsToShader(shader, 1);
        shader->SetTexture2D("diffuseTexture", 0, 0);
        renderPass(shader, RenderType::Solid);

        skyShader->Bind();
        skyShader->SetUniformMat4("projection", proj.m);
        skyShader->SetUniformMat4("view", view.m);
        skyShader->SetUniform("skybox", 0);

        renderPass(skyShader, RenderType::Sky);

        trailShader->Bind();
        trailShader->SetUniformMat4("projection", proj.m);
        trailShader->SetUniformMat4("view", view.m);
        ribbon->Render();
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");

        depthShader = ShaderManager::Instance().Load("depth", "assets/shaders/depth.vs", "assets/shaders/depth.fs");
        debugShader = ShaderManager::Instance().Load("debug", "assets/shaders/debugDepth.vs", "assets/shaders/debugDepth.fs");
        shader = ShaderManager::Instance().Load("shader", "assets/shaders/cshadow.vs", "assets/shaders/cshadow.fs");
        trailShader = ShaderManager::Instance().Load("trailShader",
                                                     "assets/shaders/effect.vs",
                                                     "assets/shaders/effect.fs");
        skyShader = ShaderManager::Instance().Load("skybox", "assets/shaders/skybox.ps", "assets/shaders/skybox.fs");

        if (!depthShader || !debugShader || !shader || !trailShader || !skyShader)
            return false;

        shadowManager = new ShadowMapManager(SHADOW_WIDTH, SHADOW_HEIGHT, CASCADE_COUNT);
        if (!shadowManager->Initialize())
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

        Mesh *skymesh = MeshManager::Instance().CreateCube("sky", 1);
        skymesh->GetBuffer(0)->Reverse();

        TextureManager::Instance().SetLoadPath("assets/cubemaps/");
        std::string files[6] = {
            "cloudy_noon_RT.jpg", // [0] POSITIVE_X = Right
            "cloudy_noon_LF.jpg", // [1] NEGATIVE_X = Left
            "cloudy_noon_UP.jpg", // [2] POSITIVE_Y = Top
            "cloudy_noon_DN.jpg", // [3] NEGATIVE_Y = Bottom
            "cloudy_noon_FR.jpg", // [4] POSITIVE_Z = Front
            "cloudy_noon_BK.jpg", // [5] NEGATIVE_Z = Back

        };

        TextureManager::Instance().SetFlipVerticalOnLoad(false);
        Texture *cubemap = TextureManager::Instance().AddCube("cubemap", files, false);
        TextureManager::Instance().SetFlipVerticalOnLoad(true);
        skymesh->AddMaterial("main")->SetTexture(0, cubemap);
        GameObject *skyObj = createGameObject("skyBOx");
        skyObj->setRenderType(RenderType::Sky);
        skyObj->addComponent<MeshRenderer>(skymesh);

        TextureManager::Instance().SetLoadPath("assets/");
        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 20, 20);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        plane = createGameObject("Plane");
        plane->addComponent<MeshRenderer>(mesh);
        plane->setPosition(0.0f, -2.5f, 0.0f);

        ribbon = new RibbonTrail(100, 4);

        Texture *ribonTex = TextureManager::Instance().Add("beam.png", false);

        // Configurar aparência
        ribbon->SetInitialColor(0, Vec3(0.2f, 0.4f, 1.0f)); // Azul brilhante
        ribbon->SetInitialColor(1, Vec3(0.2f, 0.4f, 1.0f));
        ribbon->SetInitialColor(2, Vec3(0.2f, 0.4f, 1.0f)); // Azul brilhante
        ribbon->SetInitialColor(3, Vec3(0.2f, 0.4f, 1.0f));

        ribbon->SetInitialWidth(0, 0.3f);
        ribbon->SetInitialWidth(1, 0.3f);
        ribbon->SetInitialWidth(2, 0.3f);
        ribbon->SetInitialWidth(3, 0.3f);

        ribbon->SetTrailLength(0.2f);
        ribbon->SetMinDistance(0.01f);
        ribbon->SetTexture(ribonTex);
        ribbon->SetConnectionAtachment(true);

        TextureManager::Instance().Add("sinbad/sinbad_body.tga", true);
        TextureManager::Instance().Add("sinbad/sinbad_clothes.tga", true);
        TextureManager::Instance().Add("sinbad/sinbad_sword.tga", true);

        Mesh *cube = MeshManager::Instance().CreateCube("cube", 1.0f);

        Mesh *meshSword = MeshManager::Instance().Load("sinbad_sword", "assets/sinbad/sword.h3d");

        if (meshSword)
        {
            Material *material = meshSword->GetMaterial(0);
            material->SetTexture(0, TextureManager::Instance().Get("sinbad_sword"));
        }

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
                sinbad->setPosition(1.9f, 0, 0);
                sinbad->setScale(0.5f, 0.5f, 0.5f);

                Joint3D *leftHand = sinbad->getJoint("Hand.L");
                Joint3D *rightHand = sinbad->getJoint("Hand.R");

                leftSword = createGameObject("LeftSword");
                leftSword->addComponent<MeshRenderer>(meshSword);
                leftSword->setPosition(0.8f, 0.8f, -0.5f);
                leftSword->setParent(leftHand);

                GameObject *leftSwordEnd = createGameObject("LeftSwordEnd");
                //   leftSwordEnd->addComponent<MeshRenderer>(cube);
                leftSwordEnd->setPosition(0.0f, 0.5f, 5.5f);
                leftSwordEnd->setParent(leftSword);

                rightSword = createGameObject("RightSword");
                rightSword->addComponent<MeshRenderer>(meshSword);
                rightSword->setPosition(-0.8f, 0.8f, 0.0f);
                rightSword->setParent(rightHand);

                GameObject *rightSwordEnd = createGameObject("RightSwordEnd");
                //    rightSwordEnd->addComponent<MeshRenderer>(cube);
                rightSwordEnd->setPosition(0.0f, 0.5f, 5.5f);
                rightSwordEnd->setParent(rightSword);

                ribbon->AddNode(leftSword, 0);
                ribbon->AddNode(leftSwordEnd, 1);

                ribbon->AddNode(rightSword, 2);
                ribbon->AddNode(rightSwordEnd, 3);

                // rightSword->rotateDeg(Vec3(0, 1, 0), -90.0f );
                rightSword->setEulerAnglesDeg(Vec3(-5.171500, -73.914894, -162.641190));
                leftSword->setEulerAnglesDeg(Vec3(-24.686144, 110.653992, 142.517471));
                // rightSword->rotateDeg(Vec3(0, 0, 1), -90.0f );

                // INFO: [15:27:37]: [MeshReader] Bone: Hand.L Parent(61)
                // INFO: [15:27:37]: [MeshReader] Bone: Hand.R Parent(62)

                AnimationLayer *handsLayer = animator->AddLayer();
                handsLayer->LoadAnimation("topIdle", "assets/sinbad/sinbad_HandsClosed.anim");
                handsLayer->Play("topIdle", PlayMode::Loop);

                AnimationLayer *torsoLayer = animator->AddLayer();
                torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_RunTop.anim");
                torsoLayer->LoadAnimation("topIdle", "assets/sinbad/sinbad_IdleTop.anim");
                Animation *conf = torsoLayer->LoadAnimation("topSwing", "assets/sinbad/sinbad_DrawSwords.anim");
                conf->SetTicksPerSecond(0.3);

                conf = torsoLayer->LoadAnimation("sliceVertical", "assets/sinbad/sinbad_SliceVertical.anim");
                conf->SetTicksPerSecond(0.4);
                conf = torsoLayer->LoadAnimation("sliceHorizontal", "assets/sinbad/sinbad_SliceHorizontal.anim");
                conf->SetTicksPerSecond(0.4);

                torsoLayer->Play("topIdle", PlayMode::Loop);

                animation = torsoLayer;

                AnimationLayer *legsLayer = animator->AddLayer();
                legsLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_IdleBase.anim");
                legsLayer->Play("legsRun", PlayMode::Loop);
            }

            {
                GameObject *sinbad = createGameObject("Sinbad");
                sinbad->addComponent<MeshRenderer>(meshModel);
                Animator *animator = sinbad->addComponent<Animator>();
                sinbad->setPosition(-1.9f, 0, 0);
                sinbad->setScale(0.5f, 0.5f, 0.5f);

                AnimationLayer *torsoLayer = animator->AddLayer();
                torsoLayer->LoadAnimation("topRun", "assets/sinbad/sinbad_Dance.anim");
                torsoLayer->Play("topRun", PlayMode::Loop);

                // AnimationLayer *legsLayer = animator->AddLayer();
                // legsLayer->LoadAnimation("legsRun", "assets/sinbad/sinbad_RunBase.anim");
                // legsLayer->Play("legsRun", PlayMode::Loop);
            }
        }

        return true;
    }
    void OnDestroy() override
    {
        delete ribbon;
        shadowManager->Release();
        delete shadowManager;
    }
    void OnUpdate(float dt) override
    {
        const float SPEED = 90.0f;
        time += dt;

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

        ribbon->Update(time, camera);

        if (Input::IsKeyPressed(KEY_SPACE))
        {
            animation->PlayOneShot("topSwing", "topIdle");
        }

        if (Input::IsKeyPressed(KEY_ONE))
        {
            animation->PlayOneShot("sliceVertical", "topIdle", 0.002f);
        }
        if (Input::IsKeyPressed(KEY_TWO))
        {
            animation->PlayOneShot("sliceHorizontal", "topIdle", 0.002f);
        }

        // if (Input::IsKeyDown(KEY_P))
        //     rightSword->rotateDeg(Vec3(1, 0, 0), 45.0f * dt);
        // if (Input::IsKeyDown(KEY_L))
        //     rightSword->rotateDeg(Vec3(1, 0, 0), -45.0f * dt);

        // if (Input::IsKeyDown(KEY_O))
        //     rightSword->rotateDeg(Vec3(0, 1, 0), 45.0f * dt);
        // if (Input::IsKeyDown(KEY_K))

        //     rightSword->rotateDeg(Vec3(0, 1, 0), -45.0f * dt);
        // if (Input::IsKeyDown(KEY_I))
        //     rightSword->rotateDeg(Vec3(0, 0, 1), 45.0f * dt);
        // if (Input::IsKeyDown(KEY_J))
        //     rightSword->rotateDeg(Vec3(0, 0, 1), -45.0f * dt);

        shadowManager->Update(dt);
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
