

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;

 
    Shader *waterShader;
    Shader *terrainShader;
    Shader *skyShader;
    Camera *camera;
 
    Camera *waterCamera;
    FreeCameraComponent *cameraMove;
    float mouseSensitivity{0.8f};

    RenderTarget *reflectionRT;
    RenderTarget *refractionRT;
    Texture *waterBump, *foamTexture;

    Mesh *meshWater;
    float width = 1.0f;
    float height = 1.0f;
    float yaw = 0.0f;
    Vec3 lightPos = Vec3(-2.0f, 8.0f, -4.0f);
    
    public:
    Terrain *terrain;
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
    float terrainWidth = 1000.0f;
    float terrainHeight = 1000.0f;

    float foamRange = 0.8f;
    float foamScale = 0.9f;
    float foamSpeed = 0.2f;
    float foamIntensity = 0.6f;

    float depth = 0.5f;

    Node3D *pick = nullptr;
    bool openInspector = false;

public:
    void OnRender() override
    {
        Driver &driver = Driver::Instance();
        camera->setAspectRatio((float)width / (float)height);
        const Vec3 cameraPos = camera->getPosition();
        driver.SetViewPort(0, 0, width, height);

        driver.SetCulling(CullMode::Back);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

      

        // ============================================
        // PASS 1: RENDER REFRACTION (Através da água)
        // ============================================
        {
            refractionRT->Bind();
            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

            //  câmera NORMAL (não refletida)
            SetCamera(camera);
            const Mat4 view = getViewMatrix();
            const Mat4 proj = getProjectionMatrix();
            const Vec3 camPos = camera->getPosition();

            terrainShader->Bind();
            terrainShader->SetUniformMat4("view", view.m);
            terrainShader->SetUniformMat4("projection", proj.m);
            // Lighting
            terrainShader->SetUniform("u_lightPos", lightPos.x, lightPos.y, lightPos.z);
            terrainShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            terrainShader->SetUniform("u_lightColor", 1.0f, 1.0f, 1.0f);
            // Terrain properties
            terrainShader->SetUniform("u_maxHeight", terrainMaxHeight);
            terrainShader->SetUniform("u_minHeight", terrainMinHeight);
            terrainShader->SetUniform("u_textureScale", terrainTextureScale);
            terrainShader->SetUniform("u_detailScale", terrainDetailScale);
            terrainShader->SetUniform("u_detailStrength", terrainDetailStrength);

            terrainShader->SetUniform("u_grassTexture", 0);
            terrainShader->SetUniform("u_dirtTexture", 1);
            terrainShader->SetUniform("u_rockTexture", 2);
            terrainShader->SetUniform("u_snowTexture", 3);
            terrainShader->SetUniform("u_detailMap", 4);

            terrainShader->SetUniform("useClipPlane", 1);
            terrainShader->SetUniform("clipPlane", 0.0f, -1.0f, 0.0f, 0);
            renderPass(terrainShader, RenderType::Terrain);

            sceneShader->Bind();
            sceneShader->SetUniformMat4("projection", proj.m);
            sceneShader->SetUniformMat4("view", view.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", camPos.x, camPos.y, camPos.z);

            // Renderiza apenas objetos ABAIXO da água

            renderPass(sceneShader, RenderType::Solid);

            refractionRT->Unbind();
            Texture *waterTexture = refractionRT->GetColorTexture(0);
            meshWater->SetTexture(2, waterTexture);
            Texture *waterDepthTexture = refractionRT->GetDepthTexture();
            meshWater->SetTexture(3, waterDepthTexture);
        }
        // ============================================
        // PASS 2: RENDER REFLECTION (Reflexão da água)
        // ============================================
        {

            reflectionRT->Bind();
            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
            //  driver.SetViewPort(0, 0, 1024, 1024);

            float waterPlaneY = 0.0f;

            waterCamera->setFOV(camera->getFov());
            waterCamera->setFarPlane(camera->getFar());
            waterCamera->setNearPlane(camera->getNear());
            waterCamera->setAspectRatio(camera->getAspect());

            // Get position and reflect Y
            Vec3 position = camera->getPosition();
            float distance = 2.0f * (position.y - waterPlaneY);
            position.y -= distance;
            waterCamera->setPosition(position);

            Vec3 euler = camera->getEulerAngles();
            euler.x = -euler.x; // Pitch
            waterCamera->setEulerAngles(euler);

            // Get target and reflect Y

            waterCamera->update(1.0f);
            SetCamera(waterCamera);
            const Mat4 view = getViewMatrix();
            const Mat4 proj = getProjectionMatrix();

            terrainShader->Bind();
            terrainShader->SetUniformMat4("view", view.m);
            terrainShader->SetUniformMat4("projection", proj.m);
            // Lighting
            terrainShader->SetUniform("u_lightPos", lightPos.x, lightPos.y, lightPos.z);
            terrainShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            terrainShader->SetUniform("u_lightColor", 1.0f, 1.0f, 1.0f);
            // Terrain properties
            terrainShader->SetUniform("u_maxHeight", terrainMaxHeight);
            terrainShader->SetUniform("u_minHeight", terrainMinHeight);
            terrainShader->SetUniform("u_textureScale", terrainTextureScale);
            terrainShader->SetUniform("u_detailScale", terrainDetailScale);
            terrainShader->SetUniform("u_detailStrength", terrainDetailStrength);

            terrainShader->SetUniform("u_grassTexture", 0);
            terrainShader->SetUniform("u_dirtTexture", 1);
            terrainShader->SetUniform("u_rockTexture", 2);
            terrainShader->SetUniform("u_snowTexture", 3);
            terrainShader->SetUniform("u_detailMap", 4);

            terrainShader->SetUniform("useClipPlane", 1);
            terrainShader->SetUniform("clipPlane", 0.0f, 1.0f, 0.0f, 0);

            renderPass(terrainShader, RenderType::Terrain);

            sceneShader->Bind();
            sceneShader->SetUniformMat4("projection", proj.m);
            sceneShader->SetUniformMat4("view", view.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

            renderPass(sceneShader, RenderType::Solid);

            skyShader->Bind();
            skyShader->SetUniformMat4("projection", proj.m);
            skyShader->SetUniformMat4("view", view.m);
            skyShader->SetUniform("skybox", 0);
            glDepthFunc(GL_LEQUAL);
            renderPass(skyShader, RenderType::Sky);
            glDepthFunc(GL_LESS);

            reflectionRT->Unbind();

            Texture *waterTexture = reflectionRT->GetColorTexture(0);
            meshWater->SetTexture(1, waterTexture);
        }

        {
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
            driver.SetViewPort(0, 0, width, height);
            driver.SetBlendEnable(false);
            driver.SetDepthTest(true);
            driver.SetDepthWrite(true);


             
            SetCamera(camera);
            const Mat4 view = getViewMatrix();
            const Mat4 proj = getProjectionMatrix();

            terrainShader->Bind();
            terrainShader->SetUniformMat4("view", view.m);
            terrainShader->SetUniformMat4("projection", proj.m);
            // Lighting
            terrainShader->SetUniform("u_lightPos", lightPos.x, lightPos.y, lightPos.z);
            terrainShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            terrainShader->SetUniform("u_lightColor", 1.0f, 1.0f, 1.0f);
            // Terrain properties
            terrainShader->SetUniform("u_maxHeight", terrainMaxHeight);
            terrainShader->SetUniform("u_minHeight", terrainMinHeight);
            terrainShader->SetUniform("u_textureScale", terrainTextureScale);
            terrainShader->SetUniform("u_detailScale", terrainDetailScale);
            terrainShader->SetUniform("u_waterLevel", 0.0f);
            terrainShader->SetUniform("u_time", time);

            terrainShader->SetUniform("u_waterLevel", 0.0f);
            terrainShader->SetUniform("u_underwaterFogColor", 0.0f, 0.3f, 0.5f);
            terrainShader->SetUniform("u_underwaterFogDensity", 0.1f);

            terrainShader->SetUniform("u_grassTexture", 0);
            terrainShader->SetUniform("u_dirtTexture", 1);
            terrainShader->SetUniform("u_rockTexture", 2);
            terrainShader->SetUniform("u_snowTexture", 3);
            terrainShader->SetUniform("u_detailMap", 4);
            terrainShader->SetUniform("useClipPlane", 0);

            terrainShader->SetUniform("useClipPlane", 0);

            renderPass(terrainShader, RenderType::Terrain);

            sceneShader->Bind();
            sceneShader->SetUniformMat4("projection", proj.m);
            sceneShader->SetUniformMat4("view", view.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

            renderPass(sceneShader, RenderType::Solid);

            waterShader->Bind();
            waterShader->SetUniform("waterBump", 0);
            waterShader->SetUniform("reflectionTexture", 1);
            waterShader->SetUniform("refractionTexture", 2);
            waterShader->SetUniform("refractionDepth", 3);
            waterShader->SetUniform("foamTexture", 4);

         

            waterShader->SetUniform("mult", depth);

            waterShader->SetUniformMat4("projection", proj.m);
            waterShader->SetUniformMat4("view", view.m);

            waterShader->SetUniform("u_cameraPosition", cameraPos.x, cameraPos.y, cameraPos.z);
            waterShader->SetUniform("u_waveHeight", waveHeight);
            waterShader->SetUniform("u_waterColor", 0.1f, 0.1f, 0.6f, 1.0f);
            waterShader->SetUniform("u_colorBlendFactor", colorBlendFactor);
            waterShader->SetUniform("u_waterColor", 0.1f, 0.1f, 0.6f, 1.0f);
            waterShader->SetUniform("u_windDirection", windDirection.x, windDirection.y);
            waterShader->SetUniform("u_waveHeight", 0.5f); // Amplitude geral
            waterShader->SetUniform("u_windForce", windForce);
            waterShader->SetUniform("u_waveLength", waveLength);
            waterShader->SetUniform("u_time", time);
            waterShader->SetUniform("u_wave1", 1.0f, 0.0f, 0.5f, 10.0f);
            waterShader->SetUniform("u_wave2", 0.7f, 0.7f, 0.3f, 6.0f);
            waterShader->SetUniform("u_wave3", 0.0f, 1.0f, 0.2f, 3.0f);
            waterShader->SetUniform("u_wave4", -0.5f, 0.5f, 0.15f, 2.0f);

            waterShader->SetUniform("u_foamRange", foamRange);
            waterShader->SetUniform("u_foamScale", foamScale);
            waterShader->SetUniform("u_foamSpeed", foamSpeed);
            waterShader->SetUniform("u_foamIntensity", foamIntensity);

            driver.SetBlendEnable(true);
            driver.SetDepthTest(true);
            driver.SetDepthWrite(false);
            driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
  
    
            renderPass(waterShader, RenderType::Water);
            

            driver.SetBlendEnable(false);
            driver.SetDepthTest(true);
            driver.SetDepthWrite(true);
            
            skyShader->Bind();
            skyShader->SetUniformMat4("projection", proj.m);
            skyShader->SetUniformMat4("view", view.m);
            skyShader->SetUniform("skybox", 0);
            glDepthFunc(GL_LEQUAL);
            renderPass(skyShader, RenderType::Sky);
            glDepthFunc(GL_LESS);
        }

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

        // debugShader->Bind();
        // debugShader->SetUniform("tex", 0);
        // debugShader->SetUniform("isDepth", 0);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D,refractionRT->GetColorTextureID());
        // Driver::Instance().DrawScreenQuad(5, 0, 200, 200);

        // debugShader->SetUniform("isDepth", 1);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D,refractionRT->GetDepthTextureID());
        // Driver::Instance().DrawScreenQuad(225, 0, 200, 200);

        // debugShader->SetUniform("isDepth", 0);
        // debugShader->SetUniform("tex", 0);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, reflectionRT->GetColorTextureID());
        // Driver::Instance().DrawScreenQuad(820, 0, 200, 200);
    }

    void OnLoad(Object *object) 
    {

    }
    bool OnCreate() override
    {

       

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
          waterShader = ShaderManager::Instance().Load("water", "assets/shaders/water.ps", "assets/shaders/water.fs");
        terrainShader = ShaderManager::Instance().Load("terrain",
                                                       "assets/shaders/terrain.ps",
                                                       "assets/shaders/terrain.fs");
        skyShader = ShaderManager::Instance().Load("skybox", "assets/shaders/skybox.ps", "assets/shaders/skybox.fs");

        if (!sceneShader  || !waterShader || !terrainShader || !skyShader)
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

        waterCamera = createCamera("WaterCamera");
        waterCamera->setAspectRatio((float)screenWidth / (float)screenHeight);
        waterCamera->setFOV(45.0f);
        waterCamera->setNearPlane(0.1f);
        waterCamera->setFarPlane(1000.0f);
        waterCamera->setPosition(0.0f, 0.5f, 10.0f);
        waterCamera->update(1);

   
        waterCamera->copyFrom(camera);

        terrain = MeshManager::Instance().CreateTerrain("terrain", "assets/terrain-heightmap.png",   
                           1.1f,   // scaleX
                           20.0f,  // scaleY (altura)
                           1.1f,   // scaleZ
                           20.0f,  // texScaleU
                           20.0f); // texScaleV
 
        if (!terrain)
        {
            LogError("[Main] Failed to create terrain");
            return false;
        }

   

        TextureManager::Instance().SetLoadPath("assets/");
       
        terrain->SetTexture(0, TextureManager::Instance().Add("terr_dirt-grass.jpg", true)); // grass
        terrain->SetTexture(1, TextureManager::Instance().Add("terr_rock-dirt.jpg", true));  // dirt
        terrain->SetTexture(2, TextureManager::Instance().Add("terr_rock6.jpg", true));      // rock
        terrain->SetTexture(3, TextureManager::Instance().Add("snow_1024.jpg", true));       // snow
        terrain->SetTexture(4, TextureManager::Instance().Add("detailmap3.jpg", true));      // detail 4 all

        


        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);
        waterBump = TextureManager::Instance().Add("waterbump.png", true);
        foamTexture = TextureManager::Instance().Add("foam_shore.png", true);

        // meshWater = MeshManager::Instance().CreatePlane("water", 200.0f, 200.0f, 528, 528, 100.0f, 100.0f);

        meshWater = MeshManager::Instance().CreateHillPlane(
            "water",
            terrainWidth,
            terrainHeight,
            128,
            128,
            0.0f,
            8.0f, // Muitas ondas em X
            8.0f, // Muitas ondas em Y
            2.0f,
            2.0f);

        meshWater->AddMaterial("reflection");
        meshWater->SetTexture(0, waterBump);
        meshWater->SetTexture(4, foamTexture);

        GameObject *water = createGameObject("water");
        water->addComponent<MeshRenderer>(meshWater);
        water->setPosition(0.0f, 0.0f, 0.0f);
        water->setRenderType(RenderType::Water);

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
        Texture *cubemap = TextureManager::Instance().AddCube("cubemap", files, false);
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

        {
            GameObject *terrainObj = createGameObject("terrain");
            terrainObj->setRenderType(RenderType::Terrain);
            terrainObj->addComponent<TerrainRenderer>(terrain);
            terrainObj->setPosition(0, -5, 0); // Abaixo da água
        }

        
        auto &rtMgr = RenderTargetManager::Instance();

        // Render target para REFLEXÃO
        reflectionRT = rtMgr.Create("WaterReflection", 640, 360);
        reflectionRT->AddColorAttachment(TextureFormat::RGBA8);
        reflectionRT->AddDepthAttachment(TextureFormat::DEPTH24);

        if (!reflectionRT->Finalize())
        {
            LogError("Failed to finalize reflection RT!");
            return false;
        }

        refractionRT = rtMgr.Create("WaterRefraction", 640, 360);
        refractionRT->AddColorAttachment(TextureFormat::RGBA8);
        refractionRT->AddDepthTexture(TextureFormat::DEPTH24);

        if (!refractionRT->Finalize())
        {
            LogError("Failed to finalize refraction RT!");
            return false;
        }

        rtMgr.PrintStats();


 
        
       

        return true;
    }
    
    void OnDestroy() override
    {
        
    }
    void OnUpdate(float dt) override
    {

        time += dt * 0.5f;

        // ANIMAR DIREÇÃO DO VENTO
        windDirection.x = cosf(time * 0.02f);
        windDirection.y = sinf(time * 0.02f);

        //    ANIMAR FORÇA DO VENTO (pulsação)
        windForce = 1.0f + sinf(time) * 0.005f;

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


    GameObject *terrain = scene.getGameObjectByName("terrain");

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


      ///  scene.terrain->Debug(&batch);

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
