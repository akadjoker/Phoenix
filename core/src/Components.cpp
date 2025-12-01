#include "pch.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Math.hpp"
#include "Node.hpp"
#include "Node3D.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"

void MeshRenderer::render(Shader *shader)
{
    if (!visible || !isEnabled())
        return;
    if (mesh)
    {
        if (mesh->IsSkinned())
            UpdateSkinning();

        const Mat4 &mat =m_owner->getWorldTransform(); 
        shader->SetModelMat4(mat.m);

        // LogInfo("[MeshRenderer] render %s", mesh->getName().c_str());
        Driver::Instance().DrawMesh(mesh);
    }
}

void MeshRenderer::UpdateSkinning()
{
    if (!mesh || !mesh->IsSkinned())
        return;

    for (size_t i = 0; i < mesh->GetBufferCount(); ++i)
    {
        MeshBuffer *buf = mesh->GetBuffer(i);
        if (!buf->IsSkinned())
            continue;

        buf->UpdateSkinning(boneMatrices);
    }
}

MeshRenderer::MeshRenderer(Mesh *m) : mesh(m), visible(true)
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::attach()
{

    m_owner->getWorldTransform();
    if (mesh && mesh->HasSkeleton())
    {
        boneMatrices.resize(mesh->GetBoneCount(), Mat4::Identity());
        m_owner->CreateJoints(mesh);
    }

    m_owner->m_boundBox.expand(mesh->getBoundingBox());
}

//******************************************* */

void MeshM3DRenderer::render( Shader *shader)
{
    if (!visible || !isEnabled())
        return;
    if (mesh)
    {
        const Mat4 &mat =m_owner->getWorldTransform(); 
        shader->SetModelMat4(mat.m);

        mesh->Render();
    }
}

MeshM3DRenderer::MeshM3DRenderer(MeshM3D *m) : mesh(m), visible(true)
{
}

MeshM3DRenderer::~MeshM3DRenderer()
{
}

void MeshM3DRenderer::attach()
{

    m_owner->getWorldTransform();
    m_owner->m_boundBox.expand(mesh->getBoundingBox());

    if (mesh)
    {
        for (u32 i = 0; i < mesh->GetBoneCount(); i++)
        {
            const MeshM3D::Bone *bone = mesh->GetBone(i);
            Joint3D *joint = m_owner->addJoint(bone->name);
            joint->setParent(m_owner);
        }
    }
}
 

void LookAtCamera::lateUpdate(float deltaTime)
{
    if (!isEnabled() || !targetCamera)
        return;

    GameObject *owner = getOwner();
    if (owner)
    {
        Vec3 cameraPos = targetCamera->getPosition();
        owner->lookAt(cameraPos);
    }
}

FreeCameraComponent::FreeCameraComponent()
    : Component(), m_camera(nullptr), m_moveSpeed(10.0f), m_sprintMultiplier(2.0f), m_slowMultiplier(0.25f), m_mouseSensitivity(0.1f), m_pitch(0.0f), m_yaw(0.0f), m_invertY(false), m_minPitch(-89.0f), m_maxPitch(89.0f), m_moveInput(0, 0, 0), m_rotationInput(0, 0), m_isSprinting(false), m_isSlowMode(false)
{
}

void FreeCameraComponent::attach()
{
    // Tentar obter Camera do GameObject
    m_camera = dynamic_cast<Camera *>(getOwner());

    if (m_camera)
    {
        // Inicializar pitch/yaw a partir da rotação atual
        Vec3 euler = m_camera->getEulerAnglesDeg();
        m_pitch = euler.x;
        m_yaw = euler.y;
    }
}

void FreeCameraComponent::update(float dt)
{
    if (!m_camera)
    {
        LogWarning(" No cmara atach!");
        return;
    }
    updateRotation(dt);
    updateMovement(dt);
}

void FreeCameraComponent::updateRotation(float dt)
{
    // Aplicar input de rotação
    float yawDelta = m_rotationInput.x * m_mouseSensitivity;
    float pitchDelta = m_rotationInput.y * m_mouseSensitivity * (m_invertY ? 1.0f : -1.0f);

    m_yaw += yawDelta;
    m_pitch += pitchDelta;

    // Clamp pitch
    m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);

    // Aplicar rotação à câmera
    // Quat rotation = Quat::FromEulerAnglesDeg(m_yaw, 0.0, m_pitch);
    Quat rotation = Quat::FromEulerAnglesDeg(m_pitch, m_yaw, 0.0f);

    m_camera->setRotation(rotation, TransformSpace::World);

    // Reset input de rotação
    m_rotationInput = Vec2(0, 0);
}

void FreeCameraComponent::updateMovement(float dt)
{
    if (m_moveInput.lengthSquared() < 0.0001f)
        return;

    // Calcular velocidade
    float speed = m_moveSpeed;
    if (m_isSprinting)
        speed *= m_sprintMultiplier;
    if (m_isSlowMode)
        speed *= m_slowMultiplier;

    // Normalizar input se necessário (diagonal não deve ser mais rápido)
    Vec3 input = m_moveInput;
    float inputLength = input.length();
    if (inputLength > 1.0f)
        input = input / inputLength;

    // Calcular movimento em world space baseado na orientação da câmera
    Vec3 forward = m_camera->getForward(TransformSpace::World);
    Vec3 right = m_camera->getRight(TransformSpace::World);
    // Vec3 up = Vec3(0, 1, 0);  // Up sempre no mundo para 6DOF puro

    // Para 6DOF verdadeiro,  o up da câmera:
    Vec3 up = m_camera->getUp();

    Vec3 movement = (right * input.x + up * input.y + forward * input.z) * speed * dt;

    m_camera->translate(movement, TransformSpace::World);
}

void FreeCameraComponent::setMoveInput(const Vec3 &input)
{
    m_moveInput = input;
}

void FreeCameraComponent::setRotationInput(const Vec2 &delta)
{
    m_rotationInput.x -= delta.x;
    m_rotationInput.y -= delta.y;
}

void FreeCameraComponent::setPitchLimits(float minDeg, float maxDeg)
{
    m_minPitch = minDeg;
    m_maxPitch = maxDeg;
}

void FreeCameraComponent::setPitch(float pitchDeg)
{
    m_pitch = std::clamp(pitchDeg, m_minPitch, m_maxPitch);

    Quat rotation = Quat::FromEulerAnglesDeg(m_pitch, m_yaw, 0.0f);
    m_camera->setRotation(rotation, TransformSpace::World);
}

void FreeCameraComponent::setYaw(float yawDeg)
{
    m_yaw = yawDeg;

    Quat rotation = Quat::FromEulerAnglesDeg(m_pitch, m_yaw, 0.0f);
    m_camera->setRotation(rotation, TransformSpace::World);
}

// VertexAnimator

VertexAnimator::VertexAnimator() : Component()
{
    currentTime = 0.0f;
    transitionTime = 0.0f;
    transitionDuration = 0.3f;
    m_mesh = nullptr;
    meshRenderer = nullptr;
    state = VertexAnimator::State::STOPPED;
    m_active = false;
}

VertexAnimator::~VertexAnimator()
{
    for (auto &animation : animations)
    {
        delete animation.second;
    }
    animations.clear();
}

void VertexAnimator::attach()
{
    if (!m_owner->hasComponent<MeshM3DRenderer>())
    {
        LogError("[Animator] %s has no MeshM3DRenderer component", m_owner->getName().c_str());
        m_active = false;
        return;
    }
    meshRenderer = m_owner->getComponent<MeshM3DRenderer>();
    if (!meshRenderer)
    {
        LogError("[Animator] %s has no MeshM3DRenderer component", m_owner->getName().c_str());
        m_active = false;
        return;
    }
    m_mesh = meshRenderer->getMesh();
    if (!m_mesh)
    {
        LogError("[Animator] %s has no mesh", m_owner->getName().c_str());
        m_active = false;
        return;
    }

    m_active = true;
}

void VertexAnimator::AddAnimation(const std::string &name, int startFrame, int endFrame, float fps, bool loop)
{
    if (!m_active)
        return;
    if (animations.find(name) != animations.end())
    {
        LogInfo("Animation '%s' already exists!\n", name.c_str());
        return;
    }

    animations[name] = new VertexAnimation(name, startFrame, endFrame, fps, loop);
}

void VertexAnimator::PlayAnimation(const std::string &name, bool forceRestart)
{
    if (!m_active)
        return;
    if (animations.find(name) == animations.end())
    {
        LogInfo("Animation '%s' not found!\n", name.c_str());
        return;
    }

    if (currentAnimation == name && !forceRestart && state == VertexAnimator::State::PLAYING)
        return;

    currentAnimation = name;
    currentTime = 0.0f;
    state = VertexAnimator::State::PLAYING;
    transitionTime = 0.0f;
    queuedAnimation.clear();
}

void VertexAnimator::PlayAnimationThen(const std::string &name, const std::string &nextAnim, bool forceRestart)
{
    PlayAnimation(name, forceRestart);
    queuedAnimation = nextAnim;
}

void VertexAnimator::TransitionToAnimation(const std::string &name, float duration)
{
    if (!m_active)
        return;
    if (animations.find(name) == animations.end())
    {
        LogInfo("Animation '%s' not found!\n", name.c_str());
        return;
    }

    if (currentAnimation == name || trasitionName == name)
        return;

    nextAnimation = name;
    transitionDuration = duration;
    transitionTime = 0.0f;
    state = TRANSITIONING;
    trasitionName = name;
    queuedAnimation.clear();
}

void VertexAnimator::Stop()
{
    state = VertexAnimator::State::STOPPED;
    currentTime = 0.0f;
    queuedAnimation.clear();
}

void VertexAnimator::Pause()
{
    state = VertexAnimator::State::PAUSED;
}

void VertexAnimator::Resume()
{
    if (state == VertexAnimator::State::PAUSED)
        state = VertexAnimator::State::PLAYING;
}

void VertexAnimator::update(float deltaTime)
{
    if (!m_active)
        return;
    if (state == VertexAnimator::State::STOPPED || currentAnimation.empty())
        return;

    if (state == VertexAnimator::State::TRANSITIONING)
        UpdateTransition(deltaTime);
    else if (state == VertexAnimator::State::PLAYING)
        UpdateAnimation(deltaTime);

    int currentFrame = 0, nextFrame = 0;
    float interpolation = 0.0f;
    GetCurrentFrames(currentFrame, nextFrame, interpolation);

    m_mesh->SetFrame(currentFrame, nextFrame, interpolation);

    for (u32 i = 0; i < m_mesh->GetBoneCount(); i++)
    {
        const MeshM3D::Bone *bone = m_mesh->GetBone(i);
        Joint3D *joint = m_owner->getJoint(i);
        joint->setLocalTransform(bone->transform);
        joint->update(deltaTime);
    }
}

void VertexAnimator::UpdateAnimation(float deltaTime)
{
    if (!m_active)
        return;
    if (animations.find(currentAnimation) == animations.end())
        return;

    VertexAnimation *anim = animations[currentAnimation];
    currentTime += deltaTime;

    float frameDuration = 1.0f / anim->fps;
    int totalFrames = anim->endFrame - anim->startFrame + 1;
    float totalDuration = totalFrames * frameDuration;

    if (anim->loop)
    {
        if (currentTime >= totalDuration)
            currentTime = fmod(currentTime, totalDuration);
    }
    else
    {
        if (currentTime >= totalDuration)
        {
            currentTime = totalDuration - frameDuration;

            std::string completedAnim = currentAnimation;

            if (!queuedAnimation.empty())
                PlayAnimation(queuedAnimation);
            else
                state = STOPPED;
        }
    }
}

void VertexAnimator::UpdateTransition(float deltaTime)
{
    if (!m_active)
        return;
    transitionTime += deltaTime;

    if (transitionTime >= transitionDuration)
    {
        currentAnimation = nextAnimation;
        nextAnimation.clear();
        currentTime = 0.0f;
        state = PLAYING;
        transitionTime = 0.0f;
    }
}

void VertexAnimator::GetCurrentFrames(int &currentFrame, int &nextFrame, float &interpolation)
{
    if (!m_active)
        return;
    if (currentAnimation.empty())
    {
        currentFrame = nextFrame = 0;
        interpolation = 0.0f;
        return;
    }

    if (state == TRANSITIONING && !currentAnimation.empty() && !nextAnimation.empty())
    {
        GetTransitionFrames(currentFrame, nextFrame, interpolation);
        return;
    }

    VertexAnimation *anim = animations[currentAnimation];

    float frameDuration = 1.0f / anim->fps;

    float frameFloat = currentTime / frameDuration;
    int frameIndex = (int)frameFloat;
    interpolation = frameFloat - frameIndex;

    if (state == STOPPED)
    {
        //  LogInfo("Animation '%s' is %d stop !\n", currentAnimation.c_str(),lastFrame);
        currentFrame = nextFrame = lastFrame;
        return;
    }
    currentFrame = anim->startFrame + frameIndex;
    lastFrame = currentFrame;

    nextFrame = currentFrame + 1;
    m_current_frame = currentFrame;

    if (nextFrame > anim->endFrame)
    {
        if (anim->loop)
        {
            nextFrame = anim->startFrame;
        }
        else
        {

            nextFrame = anim->endFrame;
            interpolation = 0.0f;
        }
    }
}

void VertexAnimator::GetTransitionFrames(int &currentFrame, int &nextFrame, float &interpolation)
{
    float transitionRatio = transitionTime / transitionDuration;

    VertexAnimation *currentAnim = animations[currentAnimation];
    VertexAnimation *nextAnim = animations[nextAnimation];

    float frameDuration = 1.0f / currentAnim->fps;
    float frameFloat = currentTime / frameDuration;
    int frameIndex = (int)frameFloat;

    currentFrame = currentAnim->startFrame + frameIndex;
    if (currentFrame > currentAnim->endFrame)
    {
        if (currentAnim->loop)
            currentFrame = currentAnim->startFrame + (frameIndex % (currentAnim->endFrame - currentAnim->startFrame + 1));
        else
            currentFrame = currentAnim->endFrame;
    }

    nextFrame = nextAnim->startFrame;
    interpolation = transitionRatio;
}

// GETTERS

bool VertexAnimator::IsPlaying() const
{
    return state == PLAYING || state == TRANSITIONING;
}

bool VertexAnimator::IsPlaying(const std::string &name) const
{
    return currentAnimation == name && (state == PLAYING || state == TRANSITIONING);
}

std::string VertexAnimator::GetCurrentAnimation() const
{
    return currentAnimation;
}

std::string VertexAnimator::GetQueuedAnimation() const
{
    return queuedAnimation;
}

u32 VertexAnimator::GetAnimationCount() const
{
    return animations.size();
}

u32 VertexAnimator::GetFrame() const
{
    return m_current_frame;
}

void VertexAnimator::SetTransitionDuration(float duration)
{
    transitionDuration = duration;
}

bool VertexAnimator::HasQueuedAnimation() const
{
    return !queuedAnimation.empty();
}