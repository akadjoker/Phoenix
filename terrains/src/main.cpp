

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;

    Shader *terrainShader;
    Shader *skyShader;
    Camera *camera;

    FreeCameraComponent *cameraMove;
    float mouseSensitivity{0.8f};

    float width = 1.0f;
    float height = 1.0f;
    float yaw = 0.0f;
    Vec3 lightPos = Vec3(-2.0f, 8.0f, -4.0f);

public:
    Terrain *terrain;
    
    float colorBlendFactor = 0.2f;
 
    float time = 0.0f;

    float terrainMaxHeight = 30.0f;
    float terrainMinHeight = 0.0f;
    float terrainTextureScale = 1.0f;   // Base textures
    float terrainDetailScale = 32.0f;    // Detail repete muito
    float terrainDetailStrength = 0.5f; // 50% de intensidade
    float terrainWidth = 1000.0f;
    float terrainHeight = 1000.0f;

    float foamRange = 0.8f;
    float foamScale = 0.9f;
    float foamSpeed = 0.2f;
    float foamIntensity = 0.6f;

    float depth = 0.5f;

    Node3D *pick = nullptr;
    bool openInspector = false;
       TerrainLod  *lod;

public:
    void OnDebug(RenderBatch *batch) override
    {
        //lod->debug(batch);
    }

    void OnRender() override
    {
        Driver &driver = Driver::Instance();
        camera->setAspectRatio((float)width / (float)height);
        const Vec3 cameraPos = camera->getPosition();
        driver.SetViewPort(0, 0, width, height);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
        driver.SetViewPort(0, 0, width, height);
        driver.SetCulling(CullMode::Back);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);
        driver.SetDepthTest(true);
        driver.SetDepthWrite(true);

        SetCamera(camera);
        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();


        terrainShader->Bind();
        terrainShader->SetUniformMat4("view", view.m);
        terrainShader->SetUniformMat4("projection", proj.m);

        terrainShader->SetUniform("textureScale", terrainTextureScale);
        terrainShader->SetUniform("detailScale", terrainDetailScale);
        terrainShader->SetUniform("detailStrength", terrainDetailStrength);
        terrainShader->SetUniform("basicTexture",  0);
        terrainShader->SetUniform("detailTexture", 1);
        terrainShader->SetUniform("useClipPlane", 0);


        renderPass(terrainShader, RenderType::Terrain);

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        renderPass(sceneShader, RenderType::Solid);

        driver.SetBlendEnable(false);
        driver.SetDepthTest(true);
        driver.SetDepthWrite(true);

        skyShader->Bind();
        skyShader->SetUniformMat4("projection", proj.m);
        skyShader->SetUniformMat4("view", view.m);
        skyShader->SetUniform("skybox", 0);
 
        renderPass(skyShader, RenderType::Sky);
 

        if (Input::IsMousePressed(MouseButton::RIGHT))
        {
            int x = Input::GetMouseX();
            int y = Input::GetMouseY();

            if (pick != nullptr)
            {
                pick->setShowBoxes(false);
                pick = nullptr;
            }

            if (Pick(RenderType::Solid, x, y, &pick))
            {

                pick->setShowBoxes(true);
                LogInfo("Picked %s", pick->getName().c_str());
            }
        }
    }

    void
    OnLoad(Object *object)
    {
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
        terrainShader = ShaderManager::Instance().Load("terrain",
                                                       "assets/shaders/landscape.vs",
                                                       "assets/shaders/landscape.fs");
        skyShader = ShaderManager::Instance().Load("skybox", "assets/shaders/skybox.ps", "assets/shaders/skybox.fs");

        if (!sceneShader || !terrainShader || !skyShader)
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
        camera->setPosition(0.0f, 10.5f, 40.0f);

        lod = createTerrainLod("terrainLOD", "assets/terrain-heightmap.png",
                                     5,               // maxLOD
                                     PATCH_17,       // patchSize
                                     Vec3(0, 0, 0), // position
                        
                                     Vec3(40.0f, 2.4f, 40.0f)   // scale
        );

        terrain = createTerrain("terrain", "assets/terrain-heightmap.png",
                                0.2f,   // scaleX
                                0.5f,  // scaleY (altura)
                                0.2f,   // scaleZ
                                1.0f,  // texScaleU
                                1.0f); // texScaleV

        if (!terrain)
        {
            LogError("[Main] Failed to create terrain");
            return false;
        }
        //terrain->setPosition(0, -5, 0);

        terrain->setActive(false);
        //lod->setActive(false);
        TextureManager::Instance().SetLoadPath("assets/");

        terrain->GetMaterial()->SetTexture(0, TextureManager::Instance().Add("terrain-texture.jpg", true));  
        terrain->GetMaterial()->SetTexture(1, TextureManager::Instance().Add("detailmap3.jpg", true));       

        lod->GetMaterial()->SetTexture(0, TextureManager::Instance().Add("terrain-texture.jpg", true));  
        lod->GetMaterial()->SetTexture(1, TextureManager::Instance().Add("detailmap3.jpg", true));       


        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));

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

        TextureManager::Instance().SetLoadPath("assets/");
        Mesh *tree = MeshManager::Instance().Load("Tree", "assets/trees/tree3.h3d");
        tree->AddMaterial("tree");
        tree->SetTexture(0, TextureManager::Instance().Add("trees/BarkDecidious0143_5_S.jpg", true));

        {
            GameObject *skyObj = createGameObject("skyBOx");
            skyObj->setRenderType(RenderType::Sky);
            skyObj->addComponent<MeshRenderer>(skymesh);
        }

        return true;
    }

    void OnDestroy() override
    {
    }
    void OnUpdate(float dt) override
    {

        time += dt * 0.5f;

        const float SPEED = 10.0f;

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
    }

    void OnResize(u32 w, u32 h) override
    {
        width = w;
        height = h;
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
        scene.Release();
        device.Close();
        return 1;
    }
    scene.OnResize(device.GetWidth(), device.GetHeight());

    // GameObject *terrain = scene.getGameObjectByName("terrain");

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

        // driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        scene.Render();

        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        //  batch.Grid(10, 1.0f, true);

        scene.Debug(&batch);

        //  batch.Box(terrain->getTransformedBoundingBox());

        //  scene.terrain->Debug(&batch);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

        // Stats window
        gui.BeginWindow("Stats", screenWidth - 260, 10, 280, 170);
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

        if (Input::IsKeyPressed(KEY_F12))
        {

            char filename[256];
            time_t now = time(nullptr);
            tm *timeinfo = localtime(&now);
            strftime(filename, sizeof(filename), "screenshots/screenshot_%Y%m%d_%H%M%S.png", timeinfo);

            device.TakeScreenshot(filename);
            LogInfo("Screenshot saved: %s", filename);
        }

        device.Flip();
    }

    scene.Release();
    font.Release();
    batch.Release();
    device.Close();

    return 0;
}
