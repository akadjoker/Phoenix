

#include "Core.hpp"
#include "godrays.hpp"

int screenWidth = 1024;
int screenHeight = 720;

class MainScene : public Scene
{
    Shader *sceneShader;

 
    Camera *camera;

    FreeCameraComponent *cameraMove;
    float mouseSensitivity{0.8f};

    GodRays godRays;

    float width = 1.0f;
    float height = 1.0f;
    float yaw = 0.0f;
    Vec3 lightPos = Vec3(-1.9, 10.3, 27.7);

public:
    float windForce = 20.0f;
    Vec2 windDirection = Vec2(1.0f, 0.0f);
    float waveHeight = 0.3f;
    float colorBlendFactor = 0.2f;
    float waveLength = 0.1f;
    float time = 0.0f;

    float terrainMaxHeight = 30.0f;
    float terrainMinHeight = 0.0f;
    float terrainTextureScale = 0.1f;   // Base textures
    float terrainDetailScale = 2.0f;    // Detail repete muito
    float terrainDetailStrength = 0.5f; // 50% de intensidade
    float terrainWidth = 100.0f;
    float terrainHeight = 100.0f;

    float depth = 0.5f;

    Node3D *pick = nullptr;
    bool openInspector = false;

    GameObject* sun;

public:
    void OnRender() override
    {
        Driver &driver = Driver::Instance();
        camera->setAspectRatio((float)width / (float)height);
        const Vec3 cameraPos = camera->getPosition();
        driver.SetViewPort(0, 0, width, height);

        sun->setPosition(lightPos);

        driver.SetCulling(CullMode::Back);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
        driver.SetViewPort(0, 0, width, height);
        driver.SetBlendEnable(false);
        driver.SetDepthTest(true);
        driver.SetDepthWrite(true);

        SetCamera(camera);
        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();

        godRays.BeginScenePass();

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        sceneShader->SetUniform("useClipPlane", 0);

        renderPass(sceneShader, RenderType::Solid);

        driver.SetBlendEnable(false);
        driver.SetDepthTest(true);
        driver.SetDepthWrite(true);
 

        godRays.RenderOcclusion(this, lightPos, cameraPos, view, proj);
        godRays.ApplyGodRays(lightPos, view, proj);
        godRays.Composite();

       

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

    void OnLoad(Object *object)
    {
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
 
        if (!sceneShader  )
            return false;

        if (!godRays.Init(screenWidth, screenHeight))
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

        TextureManager::Instance().SetLoadPath("assets/textures/");
        Mesh *sponsa = MeshManager::Instance().Load("sponsa", "assets/sponza.h3d");

        //   MeshManager::Instance().Save("terrain.h3d", terrain);

        TextureManager::Instance().SetLoadPath("assets/");

        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));

        Mesh *sphere = MeshManager::Instance().CreateSphere("Sphere", 10.0f, 16);
        sphere->AddMaterial("white")->SetTexture(0, TextureManager::Instance().Get("white"));

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

        // {
        //     GameObject *skyObj = createGameObject("skyBOx");
        //     skyObj->setRenderType(RenderType::Sky);
        //     skyObj->addComponent<MeshRenderer>(skymesh);
        // }

        {
            sun = createGameObject("sun");

            sun->addComponent<MeshRenderer>(sphere);
            sun->setPosition(lightPos);
            sun->setRenderType(RenderType::Special);
        }

        {
            GameObject *obj = createGameObject("spomnsa");
            obj->addComponent<MeshRenderer>(sponsa);
            obj->setRenderType(RenderType::Solid);
            obj->setScale(Vec3(0.04f));
        }

        Load("assets/water.txt");

        return true;
    }

    void OnDestroy() override
    {
    }
    void OnUpdate(float dt) override
    {

        float pulse = 0.8f + 0.2f * sinf(time * 0.5f);
    //godRays.exposure = 0.01f * pulse;

        time += dt * 0.5f;

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

    void OnGui(GUI &gui)
    {
        if (pick)
        {
            ShowNode3DInspector(gui, pick);
        }
        

            // PAINEL GOD RAYS
            bool openGodRays = true;
            if (gui.BeginWindow("God Rays", 10, 200, 280, 380, &openGodRays))
            {
                float y = 30.0f;
                float x = 10.0f;
                float w = gui.GetWindowContentWidth() - 20.0f;
                float h = 20.0f;

                gui.Text(10, 10, "God Rays Controls");

                // Exposure - Intensidade geral
                if (gui.DragFloat("Exposure", &godRays.exposure, 0.01f, 0.0f, 2.0f, x, y, w, h))
                {
                    // Valor mudou
                }
                y += h + 4.0f;

                // Decay - Quão longe vão os raios
                if (gui.DragFloat("Decay", &godRays.decay, 0.001f, 0.8f, 0.999f, x, y, w, h))
                {
                    // Valor mudou
                }
                y += h + 4.0f;

                // Density - Densidade/espaçamento dos raios
                if (gui.DragFloat("Density", &godRays.density, 0.05f, 0.1f, 3.0f, x, y, w, h))
                {
                    // Valor mudou
                }
                y += h + 4.0f;

                // Weight - Peso/contribuição de cada sample
                if (gui.DragFloat("Weight", &godRays.weight, 0.01f, 0.0f, 2.0f, x, y, w, h))
                {
                    // Valor mudou
                }
                y += h + 4.0f;

                // Samples - Qualidade (mais = melhor mas mais lento)
                float samples = (float)godRays.numSamples;
                if (gui.DragFloat("Samples", &samples, 1.0f, 20.0f, 150.0f, x, y, w, h))
                {
                    godRays.numSamples = (int)samples;
                }
                y += h + 8.0f;

                // Separador
                gui.SeparatorText("Info", 10, y, w);
                y += 25.0f;

                // Dicas
                // gui.Text(10, y, "Exposure: Brilho geral");
                // y += 18.0f;
                // gui.Text(10, y, "Decay: 0.95 = raios longos");
                // y += 18.0f;
                // gui.Text(10, y, "Density: Espaçamento");
                // y += 18.0f;
                // gui.Text(10, y, "Weight: Intensidade");
                // y += 18.0f;
                // gui.Text(10, y, "Samples: Qualidade");
                // y += 25.0f;

                // Botão Reset
                if (gui.Button("Reset Default", x, y, w, h))
                {
                    godRays.exposure = 0.3f;
                    godRays.decay = 0.95f;
                    godRays.density = 0.8f;
                    godRays.weight = 0.6f;
                    godRays.numSamples = 100;
                }

       const Color Position(230, 60, 60, 255);
             y += 25.0f;
        gui.DragFloat("Pos X", &lightPos.x, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position);
        y += h + 4.0f;
        gui.DragFloat("Pos Y", &lightPos.y, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position);
        y += h + 4.0f;
        gui.DragFloat("Pos Z", &lightPos.z, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position);
        y += h + 8.0f;

         

                gui.EndWindow();
            }
        
    }

    void ShowNode3DInspector(GUI &gui, Node3D *node)
    {
        if (!node)
            return;
        bool open = true;

        const Color Position(230, 60, 60, 255);
        const Color Rotation(60, 220, 60, 255);
        const Color Scale(80, 120, 255, 255);

        if (!gui.BeginWindow("Node Inspector", 300, 10, 260, 380, &open))
        {
            gui.EndWindow();
            return;
        }

        gui.Text(10, 5, "Node: %s", node->getName().c_str());

        float y = 60.0f;
        gui.SeparatorText("Transform", 10, y - 20, gui.GetWindowContentWidth() - 20.0f);

        float x = 10.0f;
        float w = gui.GetWindowContentWidth() - 20.0f;
        float h = 20.0f;

        Vec3 pos = node->getPosition(TransformSpace::Local);
        Vec3 rot = node->getEulerAnglesDeg();
        Vec3 scl = node->getScale(TransformSpace::Local);

        bool changedPos = false;
        bool changedRot = false;
        bool changedScl = false;

        float px = pos.x, py = pos.y, pz = pos.z;
        if (gui.DragFloat("Pos X", &px, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position))
            changedPos = true;
        y += h + 4.0f;
        if (gui.DragFloat("Pos Y", &py, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position))
            changedPos = true;
        y += h + 4.0f;
        if (gui.DragFloat("Pos Z", &pz, 0.1f, -10000.0f, 10000.0f, x, y, w, h, Position))
            changedPos = true;
        y += h + 8.0f;

        pos.x = px;
        pos.y = py;
        pos.z = pz;

        float rx = rot.x, ry = rot.y, rz = rot.z;
        if (gui.DragFloat("Rot X", &rx, 0.2f, -360.0f, 360.0f, x, y, w, h, Rotation))
            changedRot = true;
        y += h + 4.0f;
        if (gui.DragFloat("Rot Y", &ry, 0.2f, -360.0f, 360.0f, x, y, w, h, Rotation))
            changedRot = true;
        y += h + 4.0f;
        if (gui.DragFloat("Rot Z", &rz, 0.2f, -360.0f, 360.0f, x, y, w, h, Rotation))
            changedRot = true;
        y += h + 8.0f;

        rot.x = rx;
        rot.y = ry;
        rot.z = rz;

        float sx = scl.x, sy = scl.y, sz = scl.z;
        if (gui.DragFloat("Scale X", &sx, 0.01f, 0.0001f, 1000.0f, x, y, w, h, Scale))
            changedScl = true;
        y += h + 4.0f;
        if (gui.DragFloat("Scale Y", &sy, 0.01f, 0.0001f, 1000.0f, x, y, w, h, Scale))
            changedScl = true;
        y += h + 4.0f;
        if (gui.DragFloat("Scale Z", &sz, 0.01f, 0.0001f, 1000.0f, x, y, w, h, Scale))
            changedScl = true;
        y += h + 8.0f;

        scl.x = sx;
        scl.y = sy;
        scl.z = sz;

        if (changedPos)
            node->setPosition(pos, TransformSpace::Local);

        if (changedRot)
            node->setEulerAnglesDeg(rot);

        if (changedScl)
            node->setScale(scl);

        y += 10.0f;
        gui.SeparatorText("Flags", 10, y, gui.GetWindowContentWidth() - 10.0f);
        y += 20.0f;

        bool visible = node->isShowBoxes(); // só exemplo, podes trocar
        if (gui.Checkbox("Show Box", &visible, x, y, 16.0f))
            node->setShowBoxes(visible);

        gui.EndWindow();
    }

    void OnResize(u32 w, u32 h) override
    {
        width = w;
        height = h;
    }
    void OnSerialize(Serialize &obj) override
    {
        // LogInfo("OnSerialize %s" , obj.GetString("name").c_str());
        const std::string name = obj.GetString("name");
        if (name == "Cube")
        {
            GameObject *node = createGameObject("Cube");
            node->addComponent<MeshRenderer>(MeshManager::Instance().Get("Cube"));
            node->deserialize(obj);
        }
        else if (name == "Tree")
        {
            GameObject *node = createGameObject("Tree");
            node->addComponent<MeshRenderer>(MeshManager::Instance().Get("Tree"));
            node->deserialize(obj);
        }
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

    TextureManager::Instance().SetFlipVerticalOnLoad(false);
    Texture *flareTexture = TextureManager::Instance().Add("sprites.png", false);
    LensFlare lensFlare(flareTexture);

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

        lensFlare.Update(&scene, Vec3(-2.0f, 8.0f, -4.0f), cameraPos, scene.getCamera()->getDirection());

        scene.Render();

        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        //  batch.Grid(10, 1.0f, true);

        scene.Debug(&batch);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        //   lensFlare.Render(&batch, view, proj, screenWidth, screenHeight);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

        scene.OnGui(gui);

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

    scene.Save("assets/water.txt");
    scene.Release();
    font.Release();
    batch.Release();
    device.Close();

    return 0;
}
