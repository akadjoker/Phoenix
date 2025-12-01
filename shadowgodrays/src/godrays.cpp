#include "godrays.hpp"

bool GodRays::Init(int width, int height)
{

    numSamples = 100; // Mais samples = melhor qualidade
    exposure = 0.3f; // Controla intensidade final
    decay = 0.96f;    // Decay suave (0.95-0.98)
    density = 1.0f;   // Densidade do efeito
    weight = 0.5f;    // Peso de cada sample

    screenWidth = width;
    screenHeight = height;

    // Criar shaders
    occlusionShader = ShaderManager::Instance().Load("occlusion",
                                                     "assets/shaders/occlusion.vs",
                                                     "assets/shaders/occlusion.fs");

    godRaysShader = ShaderManager::Instance().Load("godRays",
                                                   "assets/shaders/godrays.vs",
                                                   "assets/shaders/godrays.fs");

    compositeShader = ShaderManager::Instance().Load("composite",
                                                     "assets/shaders/composite.vs",
                                                     "assets/shaders/composite.fs");

    if (!occlusionShader || !godRaysShader || !compositeShader)
        return false;

    CreateFramebuffers();
    return true;
}
void GodRays::CreateFramebuffers()
{
    // Scene FBO
    CHECK_GL_ERROR(glGenFramebuffers(1, &sceneFBO));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO));

    glGenTextures(1, &sceneTexture);
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, sceneTexture));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0));

    CHECK_GL_ERROR(glGenRenderbuffers(1, &sceneDepth));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth));
    CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight));
    CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepth));

    // VERIFICAR
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LogError("Scene FBO incomplete: 0x%X", status);

    // Occlusion FBO
    int occWidth = screenWidth / 2;
    int occHeight = screenHeight / 2;

    CHECK_GL_ERROR(glGenFramebuffers(1, &occlusionFBO));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, occlusionFBO));

    glGenTextures(1, &occlusionTexture);
    glBindTexture(GL_TEXTURE_2D, occlusionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, occWidth, occHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, occlusionTexture, 0);

    CHECK_GL_ERROR(glGenRenderbuffers(1, &occlusionDepth));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, occlusionDepth));
    CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, occWidth, occHeight));
    CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, occlusionDepth));

    // VERIFICAR
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LogError("Occlusion FBO incomplete: 0x%X", status);

    // God Rays FBO (SEM DEPTH - só precisa de cor)
    CHECK_GL_ERROR(glGenFramebuffers(1, &godRaysFBO));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, godRaysFBO));

    glGenTextures(1, &godRaysTexture);
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, godRaysTexture));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, occWidth, occHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, godRaysTexture, 0));

    // VERIFICAR
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LogError("God Rays FBO incomplete: 0x%X", status);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GodRays::DeleteFramebuffers()
{
    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteTextures(1, &sceneTexture);
    glDeleteRenderbuffers(1, &sceneDepth);

    glDeleteFramebuffers(1, &occlusionFBO);
    glDeleteTextures(1, &occlusionTexture);
    glDeleteRenderbuffers(1, &occlusionDepth);

    glDeleteFramebuffers(1, &godRaysFBO);
    glDeleteTextures(1, &godRaysTexture);
}

void GodRays::Resize(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
    DeleteFramebuffers();
    CreateFramebuffers();
}

void GodRays::BeginScenePass()
{
    Driver &driver = Driver::Instance();

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LogError("Scene FBO not complete in BeginScenePass: 0x%X", status);
        return;
    }

    driver.SetViewPort(0, 0, screenWidth, screenHeight);
    driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);
}

void GodRays::RenderOcclusion(Scene *scene, const Vec3 &lightWorldPos,
                              const Vec3 &cameraPos, const Mat4 &view, const Mat4 &proj)
{
    Driver &driver = Driver::Instance();

    // Verificar se sol está visível
    Vec4 lightClip = proj * view * Vec4(lightWorldPos.x, lightWorldPos.y, lightWorldPos.z, 1.0f);
    if (lightClip.w < 0.0f)
        return;

    int occWidth = screenWidth / 2;
    int occHeight = screenHeight / 2;

    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, occlusionFBO));
    driver.SetViewPort(0, 0, occWidth, occHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

    // // 1. RENDERIZAR GEOMETRIA COMO SILHUETAS PRETAS
    occlusionShader->Bind();
    occlusionShader->SetUniformMat4("view", view.m);
    occlusionShader->SetUniformMat4("projection", proj.m);
    occlusionShader->SetUniform("u_occlusionColor", 0.0f, 0.0f, 0.0f);

    driver.SetDepthTest(true);
    driver.SetDepthWrite(true);

    // Renderizar cena (sem texturas, só geometria)
  //  scene->renderPass(occlusionShader, RenderType::Terrain);
    scene->renderPass(occlusionShader, RenderType::Solid);

    occlusionShader->SetUniform("u_occlusionColor", 1.0f, 1.0f, 1.0f);
    // Disable depth testing so sun always renders on top
  //  driver.SetDepthTest(false);
  //  driver.SetDepthWrite(false); // Also disable depth writes

    float sunSize = 10.8f;
    driver.DrawBillboard(lightWorldPos, cameraPos, view, proj, sunSize);
     //   scene->renderPass(occlusionShader, RenderType::Terrain);
    //scene->renderPass(occlusionShader, RenderType::Solid);
    scene->renderPass(occlusionShader, RenderType::Special);

   // driver.SetDepthTest(true);
   // driver.SetDepthWrite(true);
}

void GodRays::ApplyGodRays(const Vec3 &lightWorldPos, const Mat4 &view, const Mat4 &proj)
{
    Driver &driver = Driver::Instance();

    Vec4 lightClip = proj * view * Vec4(lightWorldPos.x, lightWorldPos.y, lightWorldPos.z, 1.0f);
    Vec3 lightNDC = Vec3(lightClip.x, lightClip.y, lightClip.z) / lightClip.w;
    Vec2 lightScreen = Vec2(lightNDC.x * 0.5f + 0.5f, lightNDC.y * 0.5f + 0.5f);

    if (lightClip.w < 0.0f)
        return;

    int occWidth = screenWidth / 2;
    int occHeight = screenHeight / 2;

    glBindFramebuffer(GL_FRAMEBUFFER, godRaysFBO);
    driver.SetViewPort(0, 0, occWidth, occHeight);
    driver.SetDepthTest(false);
    driver.SetDepthWrite(false);

    godRaysShader->Bind();
    godRaysShader->SetUniform("u_sceneTexture", 0);
    godRaysShader->SetUniform("u_lightScreenPos", lightScreen.x, lightScreen.y);
    godRaysShader->SetUniform("u_numSamples", numSamples);
    godRaysShader->SetUniform("u_exposure", exposure);
    godRaysShader->SetUniform("u_decay", decay);
    godRaysShader->SetUniform("u_density", density);
    godRaysShader->SetUniform("u_weight", weight);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, occlusionTexture);

    driver.DrawScreenQuad();

    driver.SetDepthTest(true);
    driver.SetDepthWrite(true);
}

void GodRays::Composite()
{
    Driver &driver = Driver::Instance();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    driver.SetViewPort(0, 0, screenWidth, screenHeight);
    driver.SetDepthTest(false);
    driver.SetDepthWrite(false);
    driver.SetBlendEnable(false);

    compositeShader->Bind();
    compositeShader->SetUniform("u_sceneTexture", 0);
    compositeShader->SetUniform("u_godRaysTexture", 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, godRaysTexture);

    driver.DrawScreenQuad();

    driver.SetDepthTest(true);
    driver.SetDepthWrite(true);
}

void GodRays::Release()
{
    DeleteFramebuffers();
}
