

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

 
class MainScene : public Scene
{
    Shader *sceneShader;
    Camera *camera;

    FreeCameraComponent *cameraMove;

    float mouseSensitivity{0.8f};

        MapMesh *mapa;
  

public:
    void OnDebug(RenderBatch *batch) override {

        //    terrain->debug(batch);

    mapa->Debug(batch);

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

        const Mat4 model;

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniformMat4("model", model.m);
        sceneShader->SetUniform("uHasTexture",1);
        sceneShader->SetUniform("uHasLightmap",1);

        sceneShader->SetUniform("uTexture",0);
        sceneShader->SetUniform("uLightmap",1);



        mapa->Render();
     

         
    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/detail.vs", "assets/shaders/detail.fs");

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

        TextureManager::Instance().SetFlipVerticalOnLoad(true);


        mapa = MapMesh::CreateTestPlane(100.0f, 100.0f, 20, 20);

        // mapa->GetMaterial(0)->SetTexture(0, TextureManager::Instance().Add("terrain_texture.jpg"));
        // mapa->GetMaterial(0)->SetTexture(1, TextureManager::Instance().Add("terrain_detail.jpg"));

        TextureManager::Instance().SetLoadPath("assets/maps/textures/");

        TextureManager::Instance().SetFlipVerticalOnLoad(true);

     

 
        mapa->Load("assets/maps/city.map");

        // Pixmap tiles;
        // tiles.Load("assets/tiles.png");

        // TiledTerrain *terrain = createTiledTerrain("Terrain",8,5,8,16,1);
        // terrain->LoadTilemap(&tiles);
        // terrain->GetMaterial()->SetTexture(0, TextureManager::Instance().Add("circleTextures64.jpg"));

     

        return true;
    }
    void OnDestroy() override
    {
        delete mapa;
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
        camera->update(dt * 25.0f);
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

    TextureManager::Instance().SetFlipVerticalOnLoad(true);

  

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
        driver.SetDepthWrite(false);
        //driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);

       
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
