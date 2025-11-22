

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;
    Camera *camera;

    FreeCameraComponent *cameraMove;
    Shader *terrainShader;
    float terrainMaxHeight = 2.0f;
    float terrainMinHeight = 0.0f;
    float terrainTextureScale = 0.1f;   // Base textures
    float terrainDetailScale = 10.0f;   // Detail repete muito
    float terrainDetailStrength = 0.5f; // 50% de intensidade
    Mesh *terrain;

    float mouseSensitivity{0.8f};
    float roll = 3.0;
    Vec3 lightPos = Vec3(-2.0f, 8.0f, -4.0f);
    
    public:
    GameObject* nodeLight;
    void OnRender() override
    {

        Device &device = Device::Instance();
        Driver &driver = Driver::Instance();

        driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
        camera->setAspectRatio(device.GetWidth() / device.GetHeight());
        const Mat4 view = camera->getViewMatrix();
        const Mat4 proj = camera->getProjectionMatrix();
        const Vec3 cameraPos = camera->getPosition();
        SetCamera(camera);

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        renderAll(sceneShader);
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
        terrainShader = ShaderManager::Instance().Load("terrain",
                                                       "assets/shaders/terrain.ps",
                                                       "assets/shaders/terrain.fs");

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

        terrain = MeshManager::Instance().CreateTerrainFromHeightmap(
            "terrain",
            "assets/terrain-heightmap.png",
            500.0f, // width
            500.0f, // height
            terrainMaxHeight,
            256,   // detailX
            256,   // detailY
            20.0f, // tilesU
            20.0f  // tilesV
        );
        if (!terrain)
        {
            LogError("[Main] Failed to create terrain");
            return false;
        }

        TextureManager::Instance().SetLoadPath("assets/");
        terrain->AddMaterial("terrain");
        terrain->SetTexture(0, TextureManager::Instance().Add("terr_dirt-grass.jpg", true)); // grass
        terrain->SetTexture(1, TextureManager::Instance().Add("terr_rock-dirt.jpg", true));  // dirt
        terrain->SetTexture(2, TextureManager::Instance().Add("terr_rock6.jpg", true));      // rock
        terrain->SetTexture(3, TextureManager::Instance().Add("snow_1024.jpg", true));       // snow
        terrain->SetTexture(4, TextureManager::Instance().Add("detailmap3.jpg", true));      // detail 4 all

        TextureManager::Instance().SetLoadPath("assets/");
        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 10, 10);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        //   GameObject *plane = createGameObject("Plane");
        //  plane->addComponent<MeshRenderer>(mesh);

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

        {
            nodeLight = createGameObject("light");
            nodeLight->addComponent<MeshRenderer>(mesh);
            nodeLight->setPosition(lightPos);

            Rotator *rotator = nodeLight->addComponent<Rotator>();
            rotator->setRotationSpeed(Vec3(0, 90, 20)); // 90°/s on Y axis
        }
        for (int i = 0; i < 7; i++)
        {
            GameObject *cube = createGameObject("Cube");
            cube->addComponent<MeshRenderer>(mesh);
            cube->setPosition(cubePositions[i]);

            //   Rotator* rotator = cube->addComponent<Rotator>();
            //    rotator->setRotationSpeed(Vec3(0, 90, 20));  // 90°/s on Y axis
        }

        {
            GameObject *terrainObj = createGameObject("terrain");
            terrainObj->addComponent<MeshRenderer>(terrain);
            terrainObj->setPosition(0, -1, 0); // Abaixo da água
            terrainObj->setRenderType(RenderType::Terrain);
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
    TextureManager::Instance().SetLoadPath("assets/");
    TextureManager::Instance().SetFlipVerticalOnLoad(false);

    Texture *flareTexture = TextureManager::Instance().Add("sprites.png", false);
        TextureManager::Instance().SetFlipVerticalOnLoad(true);
    Texture *trailTex = TextureManager::Instance().Add("trail.png", false);
    Texture *ribonTex = TextureManager::Instance().Add("beam.png", false);

    LensFlare lensFlare(flareTexture);
    Shader *trailShader = ShaderManager::Instance().Load("trailShader",
                                                       "assets/shaders/effect.vs",
                                                       "assets/shaders/effect.fs");

    TrailRenderer *trail = new TrailRenderer(trailTex, 50, 2.0f);
    trail->SetWidth(0.5f, 0.05f);                  // Start large, end thin
    trail->SetColor(Vec3(1, 1, 1), Vec3(1, 0, 1)); // Yellow to red
    trail->SetMinDistance(0.1f);                   // Add point every 0.1 units

    Vec3 objectPosition = Vec3(0, 2, 0);
    float angle = 0.0f;
    float time = 0.0f;

    RibbonTrail* ribbon = new RibbonTrail(50, 2);
    ribbon->SetTrailLength(2.0f);
    ribbon->SetMinDistance(0.1f);

    // Chain 0 - Engine esquerda
    ribbon->AddNode(scene.nodeLight, 0);
    ribbon->SetChainOffset(0, Vec3(-1, 0, -2));
    ribbon->SetInitialColor(0, Vec3(1, 1.0f, 1));  // Laranja
    ribbon->SetInitialWidth(0, 0.3f);
    ribbon->SetWidthChange(0, -0.25f);

    //Chain 1 - Engine direita
    ribbon->AddNode(scene.nodeLight, 1);
    ribbon->SetChainOffset(1, Vec3(1, 0, -2));
    ribbon->SetInitialColor(1, Vec3(0, 0.5f, 1));  // Azul
    ribbon->SetInitialWidth(1, 0.3f);
    ribbon->SetWidthChange(1, -0.25f);

    ribbon->SetTexture(ribonTex);
  

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
        lensFlare.Update(&scene, Vec3(-2.0f, 8.0f, -4.0f), cameraPos, scene.getCamera()->getDirection());

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        scene.Render();



        time += dt;
        angle += dt * 2.0f;  // Rotate 2 rad/s
        
        // Move object in circular pattern
        float radius = 3.0f;
        objectPosition.x = cos(angle) * radius;
        objectPosition.z = sin(angle) * radius;
        objectPosition.y = 2.0f + sin(angle * 2.0f) * 0.5f;  // Up and down
        
        // Add point to trail
        trail->AddPoint(objectPosition, time);
        trail->Update(time);
        ribbon->Update(time, scene.getCamera());
        trailShader->Bind();
        trailShader->SetUniformMat4("projection", proj.m);
        trailShader->SetUniformMat4("view", view.m);
        trail->Render( scene.getCamera ());
        ribbon->Render( );

        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        batch.Grid(10, 1.0f, true);

        scene.Debug(&batch);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        lensFlare.Render(&batch, view, proj, screenWidth, screenHeight);

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

            // scene.getCamera()->rotate(mouseDelta.y * mouseSensitivity, mouseDelta.x * mouseSensitivity);
        }

        batch.Render();

        device.Flip();
    }

    delete ribbon;
    delete trail;
    scene.Release();
    font.Release();
    batch.Release();
    device.Close();

    return 0;
}
