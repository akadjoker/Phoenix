

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

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

    FreeCameraComponent *cameraMove;
    Camera camera;

    cameraMove = camera.addComponent<FreeCameraComponent>();
    cameraMove->setMoveSpeed(15.0f);
    cameraMove->setMouseSensitivity(0.15f);
    cameraMove->setSprintMultiplier(3.0f);
    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
    camera.setFOV(45.0f);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);

    Utils::ChangeDirectory("../");
    Shader *particleShader = ShaderManager::Instance().Load("particle",
                                                            "assets/shaders/particle.vs",
                                                            "assets/shaders/particle.fs");

    ParticleSystem *test = new ParticleSystem("Test", 100);

    test->SetContinuous(20.0f)                         // 20 partículas/seg
        ->SetShapePoint()                              // Ponto único
        ->SetSpeed(2.0f, 4.0f)                         // Velocidade
        ->SetLifetime(2.0f)                            // Vive 2 segundos
        ->SetSize(0.5f, 0.1f)                          // Começa 0.5, termina 0.1
        ->SetColor(Vec4(1, 1, 0, 1), Vec4(1, 0, 0, 0)) // Amarelo → Vermelho fade
        ->SetGravity(Vec3(0, -2, 0))                   // Gravidade leve
        ->SetSpreadAngle(30.0f)                        // Cone 30 graus
        ->Play();


    std::vector<ParticleSystem*> particles;


ParticleSystem* smoke = new ParticleSystem("Smoke", 200);

//Texture* smokeTexture = TextureManager::Instance().Load("assets/textures/smoke.png");

//smoke->SetTexture(smokeTexture)
     smoke->SetContinuous(30.0f)
     ->SetShapeSphere(0.2f)
     ->SetSpeed(1.0f, 2.0f)
     ->SetLifetime(3.0f)
     ->SetSize(0.3f, 1.5f)                          // Cresce
     ->SetColor(Vec4(0.8f, 0.8f, 0.8f, 0.8f), Vec4(0.5f, 0.5f, 0.5f, 0))  // Cinza fade
     ->AddTurbulence(0.5f, 1.0f)                    // Movimento aleatório
     ->SetDrag(0.5f)                                // Resistência do ar
     ->Play();

    smoke->setPosition(-2, 1, 0);


ParticleSystem* fire = new ParticleSystem("Fire", 300);

fire->SetContinuous(50.0f)
    ->SetShapeCircle(2.3f)
    ->SetEmissionDirection(Vec3(0, 0.1f, 0))                   // Para cima
    ->SetSpeed(2.0f, 4.0f)
    ->SetSpreadAngle(15.0f)
    ->SetLifetime(1.0f, 1.5f)
    ->SetSize(0.4f, 0.8f)                           // Cresce
    ->SetColor(Vec4(1, 1, 0, 1), Vec4(1, 0, 0, 0))   
    ->AddTurbulence(1.0f, 2.0f)
    ->Play();

fire->setPosition(-2, 0.5f, 2);  // No chão


    particles.push_back(smoke);
    particles.push_back(test);
    particles.push_back(fire);


    ParticleSystem* clickEffect = new ParticleSystem("ClickEffect", 500);
clickEffect->SetOneShot(50)
           ->SetShapeSphere(0.3f)
           ->SetSpeed(5.0f, 12.0f)
           ->SetLifetime(0.8f, 1.5f)
           ->SetSize(0.2f, 0.05f)
           ->SetColor(Vec4(1, 0.8f, 0, 1), Vec4(1, 0, 0, 0))
           ->SetGravity(Vec3(0, -9.8f, 0))
           ->SetDrag(0.3f);

           clickEffect->setPosition(-2, 0, -2);

           particles.push_back(clickEffect);

ParticleSystem* fountain = new ParticleSystem("Fountain", 500);
fountain->SetContinuous(100.0f)
        ->SetShapeCircle(0.2f)
        ->SetEmissionDirection(Vec3(0, 1, 0))
        ->SetSpeed(8.0f, 12.0f)
        ->SetSpreadAngle(25.0f)
        ->SetLifetime(2.0f, 3.0f)
        ->SetSize(0.15f, 0.08f)
        ->SetColor(Vec4(0.3f, 0.6f, 1, 0.8f), Vec4(0.2f, 0.4f, 0.8f, 0))
        ->SetGravity(Vec3(0, -15, 0))
        ->SetAutoPlay(true);

fountain->setPosition(0, 0.5f, 0);

particles.push_back(fountain);

ParticleSystem* rain = new ParticleSystem("Rain", 2000);
rain->SetContinuous(500.0f)
    ->SetShapeBox(Vec3(20, 0.1f, 20))  // Área grande
    ->SetEmissionDirection(Vec3(0, -1, 0))
    ->SetSpeed(15.0f, 20.0f)
    ->SetSpreadAngle(5.0f)  // Ligeiro ângulo
    ->SetLifetime(2.0f)
    ->SetSize(0.05f, 0.3f)  // Linha fina alongada
    ->SetColor(Vec4(0.6f, 0.6f, 0.8f, 0.6f), Vec4(0.4f, 0.4f, 0.6f, 0))
    ->SetAutoPlay(true);

rain->setPosition(0, 15, 0);  // Alto

particles.push_back(rain);


    while (device.Run())
    {

        float dt = device.GetFrameTime();

        if (device.IsResize())
        {
            driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
            camera.setAspectRatio(device.GetWidth() / device.GetHeight());
        }

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

        if (Input::IsMouseDown(MouseButton::LEFT))
        {
            Vec2 mouseDelta = Input::GetMouseDelta();
            cameraMove->setRotationInput(mouseDelta);
            clickEffect->Fire(); 

        }

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        camera.update(dt);
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();
        const Mat4 &mvp = proj * view;

        driver.SetDepthTest(true);
        driver.SetDepthWrite(false); // Não escrever no depth buffer
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        driver.SetCulling(CullMode::None); // Renderizar ambos os lados

        particleShader->Bind();
        particleShader->SetUniformMat4("u_view", view.m);
        particleShader->SetUniformMat4("u_projection", proj.m);
        particleShader->SetUniform("u_hasTexture", 0);

        for (ParticleSystem* particle : particles)
        {
            particle->RenderParticles(view, proj);
            particle->render(particleShader);
            particle->update(dt);
        }
       



        batch.SetMatrix(mvp);

        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        batch.Grid(10, 1.0f, true);

        batch.Render();

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);



        batch.SetMatrix(mvp);
        driver.SetDepthTest(true);
        driver.SetBlendEnable(false);

        batch.Grid(10, 1.0f, true);

        batch.Render();

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        batch.SetColor(255, 255, 255);

        font.Print(10, 10, "Fps %d", device.GetFPS());

        batch.Render();

        device.Flip();
    }

    for (auto p : particles)
    {
        delete p;
    }

    font.Release();
    batch.Release();

    MeshManager::Instance().UnloadAll();
    ShaderManager::Instance().UnloadAll();
    TextureManager::Instance().UnloadAll();
    device.Close();

    return 0;
}
