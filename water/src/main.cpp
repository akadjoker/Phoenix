

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;
    Shader *mirrorShader;
    Shader *debugShader;
    Shader *waterShader;
    Shader *terrainShader;
    Camera *camera;
    Camera *mirrorCamera;
    Camera *waterCamera;
    FreeCameraComponent *cameraMove;
    float mouseSensitivity{0.8f};

    RenderTarget *mirrorRT;
    RenderTarget *reflectionRT;
    RenderTarget *refractionRT;
    Texture *waterBump;
    Texture *waterFoam;

    Mesh *meshMirror;
    Mesh *meshWater;
    Mesh *terrain;
    float width = 1.0f;
    float height = 1.0f;
    float yaw = 0.0f;
    Vec3 lightPos = Vec3(-2.0f, 8.0f, -4.0f);

public:
    float mirrorFresnelPower = 0.24f;
    float mirrorFresnelMin = 0.05f;
    Vec3 mirrorTint = Vec3(0.9f, 0.95f, 1.0f);
    float mirrorFov = 45.0f;

    float windForce = 20.0f;
    Vec2 windDirection = Vec2(1.0f, 0.0f);
    float waveHeight = 0.3f;
    float colorBlendFactor = 0.2f;
    float waveLength = 0.1f;
    float time = 0.0f;

    float terrainMaxHeight = 30.0f;
    float terrainMinHeight = 0.0f;
    float terrainTextureScale = 0.1f;   // Base textures
    float terrainDetailScale = 10.0f;   // Detail repete muito
    float terrainDetailStrength = 0.5f; // 50% de intensidade
    float terrainWidth =100.0f;
    float terrainHeight=100.0f;

    float off=0.0f;

Quat reflectQuaternionXZ(const Quat& q)
{
    // Reflete em torno do plano XZ (inverte X e Z)
    return Quat(q.w, -q.x, q.y, -q.z);
}
               
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

        {

            // ============================================
            // RENDER MIRROR REFLECTION
            // ============================================
            mirrorRT->Bind();

            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
         //   driver.SetViewPort(0, 0, 1024, 1024);

            mirrorCamera->setFOV(mirrorFov);
            SetCamera(mirrorCamera);
            const Mat4 view = getViewMatrix();
            const Mat4 proj = getProjectionMatrix();
            const Vec3 cameraPos = mirrorCamera->getPosition();

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
            renderPass(terrainShader, RenderType::Terrain);

            sceneShader->Bind();
            sceneShader->SetUniformMat4("projection", proj.m);
            sceneShader->SetUniformMat4("view", view.m);
            sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
            sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

            renderPass(sceneShader, RenderType::Solid);
            

            mirrorRT->Unbind();

            Texture *mirrorTexture = mirrorRT->GetColorTexture(0);
            meshMirror->SetTexture(0, mirrorTexture);
        }

        // ============================================
        // PASS 1: RENDER REFRACTION (Através da água)
        // ============================================
        {
            refractionRT->Bind();
            driver.SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
//            driver.SetViewPort(0, 0, 1024, 1024);

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
            meshWater->SetTexture(4, waterDepthTexture);
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
            float distance  = 2.0f * ( position.y - waterPlaneY);
            position.y -=distance;
            waterCamera->setPosition(position);



                Vec3 euler = camera->getEulerAngles();
             euler.z = Pi;  
                   euler.x = -euler.x;  // Pitch
 
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

            reflectionRT->Unbind();

            Texture *waterTexture = reflectionRT->GetColorTexture(0);
            meshWater->SetTexture(3, waterTexture);
            

        }

        {
            driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
            driver.SetViewPort(0, 0, width, height);
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
            terrainShader->SetUniform("u_detailStrength", terrainDetailStrength);

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

            mirrorShader->Bind();
            mirrorShader->SetUniform("u_reflectionTexture", 0);
            mirrorShader->SetUniformMat4("projection", proj.m);
            mirrorShader->SetUniformMat4("view", view.m);
            mirrorShader->SetUniform("u_viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
            mirrorShader->SetUniform("u_fresnelPower", mirrorFresnelPower);
            mirrorShader->SetUniform("u_fresnelMin", mirrorFresnelMin);
            mirrorShader->SetUniform("u_mirrorTint", mirrorTint.x, mirrorTint.y, mirrorTint.z);

            renderPass(mirrorShader, RenderType::Mirror);

            Mat4 worldViewProj = proj * view;
            Mat4 worldReflectionViewProj = proj * waterCamera->getViewMatrix();

            waterShader->Bind();
            waterShader->SetUniform("u_waterBump", 0);     // Slot 0
            waterShader->SetUniform("u_foamTexture", 1);     // Slot 0
            waterShader->SetUniform("u_refractionMap", 2); // Slot 1
            waterShader->SetUniform("u_reflectionMap", 3); // Slot 2
            waterShader->SetUniform("u_depthMap", 4); // Slot 2
            waterShader->SetUniformMat4("projection", proj.m);
            waterShader->SetUniformMat4("view", view.m);
            waterShader->SetUniformMat4("worldViewProj", worldViewProj.m);
            waterShader->SetUniformMat4("worldReflectionViewProj", worldReflectionViewProj.m);

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
            // Onda 1: Grande, lenta
            waterShader->SetUniform("u_wave1", 1.0f, 0.0f, 0.5f, 10.0f);

            // Onda 2: Média, diagonal
            waterShader->SetUniform("u_wave2", 0.7f, 0.7f, 0.3f, 6.0f);

            // Onda 3: Pequena, rápida
            waterShader->SetUniform("u_wave3", 0.0f, 1.0f, 0.2f, 3.0f);

            // Onda 4: Detalhe
            waterShader->SetUniform("u_wave4", -0.5f, 0.5f, 0.15f, 2.0f);






            // glActiveTexture(GL_TEXTURE0);
            // glBindTexture(GL_TEXTURE_2D, waterBump->GetHandle());

            // glActiveTexture(GL_TEXTURE1);
            // glBindTexture(GL_TEXTURE_2D, waterFoam->GetHandle());

            // glActiveTexture(GL_TEXTURE2);
            // glBindTexture(GL_TEXTURE_2D, refractionRT->GetColorTextureID(0));

            // glActiveTexture(GL_TEXTURE3);
            // glBindTexture(GL_TEXTURE_2D, reflectionRT->GetColorTextureID(0));

            // glActiveTexture(GL_TEXTURE4);
            // glBindTexture(GL_TEXTURE_2D, refractionRT->GetDepthTextureID()); // 

            renderPass(waterShader, RenderType::Water);
        }

         Texture *waterDepthTexture = refractionRT->GetDepthTexture();
   

        debugShader->Bind();
        debugShader->SetUniform("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,waterDepthTexture->GetHandle());
        Driver::Instance().DrawScreenQuad(210, 0, 200, 200);

        // debugShader->SetUniform("tex", 0);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, refractionRT->GetColorTextureID());
        // Driver::Instance().DrawScreenQuad(420, 0, 200, 200);
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");
        mirrorShader = ShaderManager::Instance().Load("mirror", "assets/shaders/mirror.ps", "assets/shaders/mirror.fs");
        debugShader = ShaderManager::Instance().Load("debug", "assets/shaders/screenQuad.ps", "assets/shaders/screenQuad.fs");
        waterShader = ShaderManager::Instance().Load("water", "assets/shaders/water.ps", "assets/shaders/water.fs");
        terrainShader = ShaderManager::Instance().Load("terrain",
                                                       "assets/shaders/terrain.ps",
                                                       "assets/shaders/terrain.fs");

        if (!sceneShader || !mirrorShader || !debugShader || !waterShader || !terrainShader)
            return false;

        mirrorCamera = createCamera("MirrorCamera");
        mirrorCamera->setAspectRatio((float)1024 / (float)1024);
        mirrorCamera->setFOV(45.0f);
        mirrorCamera->setNearPlane(0.1f);
        mirrorCamera->setFarPlane(1000.0f);
        mirrorCamera->setPosition(-5.0f, 0.5f, 0.0f);
        mirrorCamera->setRotation(-90.0f, 0, 0);
        mirrorCamera->update(1);

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

        mirrorCamera->copyFrom(camera);
        waterCamera->copyFrom(camera);

        terrain = MeshManager::Instance().CreateTerrainFromHeightmap(
            "terrain",
            "assets/hole.png",
            terrainWidth, // width
            terrainHeight, // height
            terrainMaxHeight,
            26,   // detailX
            26,   // detailY
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

        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);
        waterBump = TextureManager::Instance().Add("waterbump.png", true);
        waterFoam= TextureManager::Instance().Add("foam_shore.png", true);

        meshMirror = MeshManager::Instance().CreateQuad("mirror", Vec3(1, 0, 0), 5.0f);
        meshMirror->AddMaterial("reflection");

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
        meshWater->SetTexture(1, waterFoam);
        

        GameObject *mirror = createGameObject("mirror");
        mirror->addComponent<MeshRenderer>(meshMirror);
        mirror->setPosition(-8.0f, 2.5f, 0.0f);
        mirror->setRenderType(RenderType::Mirror);

        GameObject *water = createGameObject("water");
        water->addComponent<MeshRenderer>(meshWater);
        water->setPosition(0.0f, 0.0f, 0.0f);
        water->setRenderType(RenderType::Water);

        Mesh *mesh = MeshManager::Instance().CreateCube("Cube", 1);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("wall"));

        Vec3 cubePositions[] = {
            Vec3(0.0f, 0.5f, 0.0f),
            Vec3(3.0f, 0.5f, -3.0f),
            Vec3(-3.0f, 0.5f, -3.0f),
            Vec3(5.0f, 0.5f, 2.0f),
            Vec3(-5.0f, 0.5f, 2.0f),
            Vec3(2.0f, 0.5f, -6.0f),
            Vec3(-2.0f, 0.5f, -6.0f),
        };

        {
            GameObject *terrainObj = createGameObject("terrain");
            terrainObj->setRenderType(RenderType::Terrain);
            terrainObj->addComponent<MeshRenderer>(terrain);
            terrainObj->setPosition(0, -5, 0); // Abaixo da água
        }

        for (int i = 0; i < 7; i++)
        {
            GameObject *cube = createGameObject("Cube1");
            cube->addComponent<MeshRenderer>(mesh);
            cube->setPosition(cubePositions[i]);
        }

        {
            GameObject *cube = createGameObject("Cube2");
            cube->addComponent<MeshRenderer>(mesh);
            cube->setPosition(5, 0, 5);
        }

        {
            GameObject *cube = createGameObject("Cube3");
            cube->addComponent<MeshRenderer>(mesh);
            cube->setPosition(5, 0, -5);
        }

        {
            GameObject *cube = createGameObject("Cube");
            cube->addComponent<MeshRenderer>(mesh);
            cube->addComponent<Rotator>();
            cube->setPosition(5, 1, 0);
        }
        auto &rtMgr = RenderTargetManager::Instance();

        mirrorRT = rtMgr.CreateHDR("MirrorReflection", 1024, 1024);

        if (!mirrorRT->Finalize())
        {
            LogError("Failed to finalize mirror RT!");
            return false;
        }

        // Render target para REFLEXÃO
        reflectionRT = rtMgr.Create("WaterReflection", 1024, 1024);
        reflectionRT->AddColorAttachment(TextureFormat::RGBA8);
        reflectionRT->AddDepthAttachment(TextureFormat::DEPTH24);
        
        if (!reflectionRT->Finalize())
        {
            LogError("Failed to finalize reflection RT!");
            return false;
        }

        refractionRT = rtMgr.Create("WaterRefraction", 1024, 1024);
        refractionRT->AddColorAttachment(TextureFormat::RGBA8);
        refractionRT->AddDepthTexture(TextureFormat::DEPTH24);

  

        if (!refractionRT->Finalize())
        {
            LogError("Failed to finalize refraction RT!");
            return false;
        }

      
 

        if (!mirrorRT || !reflectionRT || !refractionRT)
        {
            LogError("Failed to create render targets!");
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

        if (Input::IsKeyDown(KEY_P))
            off-=0.1f;
        if (Input::IsKeyDown(KEY_L))
            off+=0.1f;

            

          cameraMove->setMoveInput(moveInput);
    }
    void OnResize(u32 w, u32 h) override
    {
        width = w;
        height = h;
    }
     Camera *getCamera() { return camera; }
    FreeCameraComponent *getCameraControl(){return cameraMove;};
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

    float mouseSensitivity{0.8f};

    MainScene scene;
    if (!scene.Init())
    {
        scene.Release();
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

        // driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

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
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

        gui.BeginWindow("Mirror", 10, 10, 280, 150);
        float y = 10;
        y += 20;
        gui.SliderFloat("Fresnel Power", &scene.mirrorFresnelPower, 0.0f, 10.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Fresnel Min", &scene.mirrorFresnelMin, 0.0f, 1.0f, 10, y, 260, 20);
        y += 25;
        gui.SliderFloat("Fov", &scene.mirrorFov, 0.40f, 190.0f, 10, y, 260, 20);
        y += 25;
        gui.EndWindow();

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
