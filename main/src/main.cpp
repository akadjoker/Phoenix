

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

class MainScene : public Scene
{
    Shader *sceneShader;
    Camera *camera;
    Camera *mirror;
    FreeCameraComponent *cameraMove;

    float mouseSensitivity{0.8f};
    float roll=3.0;

public:
    void OnRender() override
    {


        Device &device = Device::Instance();
        Driver &driver = Driver::Instance();
        Vec3 lightPos(-2.0f, 8.0f, -4.0f);

        {



        float waterY=0.0f;

        // Vec3 target = camera->getTarget(TransformSpace::World);
      //  target.y = -target.y + 2.0f * waterY;
        //mirror->setTarget(target, TransformSpace::World);

        Vec3 euler = camera->getEulerAngles();
        euler.z = Pi;  
        mirror->setEulerAngles(euler);


        SetCamera(mirror);
        driver.SetViewPort(0,0,256,256);
        mirror->setAspectRatio(256/256);



        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();
        const Vec3 cameraPos = mirror->getPosition();

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        renderAll(sceneShader);
        }
        {
        driver.SetViewPort(256,256,device.GetWidth(),device.GetHeight());
        camera->setAspectRatio(device.GetWidth()/device.GetHeight());
        SetCamera(camera);
        const Mat4 view = getViewMatrix();
        const Mat4 proj = getProjectionMatrix();
        const Vec3 cameraPos = camera->getPosition();

        sceneShader->Bind();
        sceneShader->SetUniformMat4("projection", proj.m);
        sceneShader->SetUniformMat4("view", view.m);
        sceneShader->SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);
        sceneShader->SetUniform("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        renderAll(sceneShader);
        }


        




    }
    bool OnCreate() override
    {

        Utils::ChangeDirectory("../");
        sceneShader = ShaderManager::Instance().Load("scene", "assets/shaders/basicLight.ps", "assets/shaders/basicLight.fs");

        if (!sceneShader)
            return false;


        mirror = createCamera("mirro");

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

        mirror->copyFrom(camera);


        TextureManager::Instance().SetLoadPath("assets/");
        TextureManager::Instance().Add("wall.jpg", true);
        TextureManager::Instance().Add("marm.jpg", true);

        Mesh *mesh = MeshManager::Instance().CreatePlane("Plane", 10, 10);
        mesh->AddMaterial("wall")->SetTexture(0, TextureManager::Instance().Get("marm"));

        GameObject *plane = createGameObject("Plane");
        plane->addComponent<MeshRenderer>(mesh);

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
            GameObject *cube = createGameObject("Cube");
            cube->addComponent<MeshRenderer>(mesh);
            cube->setPosition(cubePositions[i]);

            //   Rotator* rotator = cube->addComponent<Rotator>();
            //    rotator->setRotationSpeed(Vec3(0, 90, 20));  // 90°/s on Y axis
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
    
    if (Input::IsKeyDown(KEY_W)) moveInput.z += SPEED;  // Forward
    if (Input::IsKeyDown(KEY_S)) moveInput.z -= SPEED;  // Backward
    if (Input::IsKeyDown(KEY_A)) moveInput.x -= SPEED;  // Left
    if (Input::IsKeyDown(KEY_D)) moveInput.x += SPEED;  // Right
    if (Input::IsKeyDown(KEY_Q)) moveInput.y -= SPEED;  // Down
    if (Input::IsKeyDown(KEY_E)) moveInput.y += SPEED;  // Up

    if (Input::IsKeyDown(KEY_P)) roll+=0.1f;
    if (Input::IsKeyDown(KEY_L)) roll-=0.05f;

    LogInfo(" roll %f",roll);

    
    
    cameraMove->setMoveInput(moveInput);



    }
    void OnResize(u32 w, u32 h) override
    {
          camera->setAspectRatio((float)w / (float)h);
       
        
    }
    Camera *getCamera() { return camera; }
    FreeCameraComponent *getCameraControl(){return cameraMove;};
};



// Teste Mat4 rotations
void testMat4Rotations()
{
    // Teste RotationX
    Vec3 v(0, 1, 0);  // Vetor apontando para cima
    Mat4 rotX = Mat4::RotationXDeg(90);
    Vec3 result = rotX.TransformVector(v);
    // Esperado: (0, 0, 1) - rodou de Y para Z
    printf("RotX 90deg: (0,1,0) -> (%.3f, %.3f, %.3f)\n", result.x, result.y, result.z);
    
    // Teste RotationY
    v = Vec3(1, 0, 0);  // Vetor apontando para a direita
    Mat4 rotY = Mat4::RotationYDeg(90);
    result = rotY.TransformVector(v);
    // Esperado: (0, 0, -1) - rodou de X para -Z
    printf("RotY 90deg: (1,0,0) -> (%.3f, %.3f, %.3f)\n", result.x, result.y, result.z);
    
    // Teste RotationZ
    v = Vec3(1, 0, 0);  // Vetor apontando para a direita
    Mat4 rotZ = Mat4::RotationZDeg(90);
    result = rotZ.TransformVector(v);
    // Esperado: (0, 1, 0) - rodou de X para Y
    printf("RotZ 90deg: (1,0,0) -> (%.3f, %.3f, %.3f)\n", result.x, result.y, result.z);
    
    // Teste round-trip: Quat -> Mat4 -> Quat
    Quat q1 = Quat::FromEulerAnglesDeg(30, 45, 60);
    Mat4 m = q1.toMat4();
    Quat q2 = Quat::FromMat4(m);
    
    float dot = q1.dot(q2);
    printf("Quat->Mat4->Quat dot product: %.6f (should be ~1.0 or ~-1.0)\n", dot);
}


void testMathLibrary()
{
    printf("=== Testing Euler <-> Quat ===\n");
    Vec3 euler1(0.5f, 1.2f, -0.3f);
    Quat q1 = Quat::FromEulerAngles(euler1.x, euler1.y, euler1.z);
    Vec3 euler2 = q1.toEulerAngles();
    printf("Original: (%.3f, %.3f, %.3f)\n", euler1.x, euler1.y, euler1.z);
    printf("Extracted: (%.3f, %.3f, %.3f)\n", euler2.x, euler2.y, euler2.z);
    
    printf("\n=== Testing Quat <-> Mat4 ===\n");
    Quat q = Quat::FromEulerAnglesDeg(30, 45, 60);
    Mat4 m = q.toMat4();
    Quat q2 = Quat::FromMat4(m);
    float dot = q.dot(q2);
    printf("Quat->Mat4->Quat dot: %.6f\n", std::fabs(dot));
    
    printf("\n=== Testing Matrix Rotations ===\n");
    Mat4 rotX = Mat4::RotationXDeg(90);
    Mat4 rotY = Mat4::RotationYDeg(90);
    Mat4 rotZ = Mat4::RotationZDeg(90);
    
    Vec3 vUp(0, 1, 0);
    Vec3 vRight(1, 0, 0);
    
    Vec3 rX = rotX.TransformVector(vUp);
    Vec3 rY = rotY.TransformVector(vRight);
    Vec3 rZ = rotZ.TransformVector(vRight);
    
    printf("RotX(90°) * (0,1,0) = (%.3f, %.3f, %.3f) [expect (0,0,1)]\n", rX.x, rX.y, rX.z);
    printf("RotY(90°) * (1,0,0) = (%.3f, %.3f, %.3f) [expect (0,0,-1)]\n", rY.x, rY.y, rY.z);
    printf("RotZ(90°) * (1,0,0) = (%.3f, %.3f, %.3f) [expect (0,1,0)]\n", rZ.x, rZ.y, rZ.z);
}

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
   
           // scene.getCamera()->rotate(mouseDelta.y * mouseSensitivity, mouseDelta.x * mouseSensitivity);
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
