#include "pch.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Math.hpp"
#include "Node.hpp"
#include "Node3D.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"

void MeshRenderer::render()
{
    if (!visible || !isEnabled())
        return;
    if (mesh)
    {
       Driver::Instance().DrawMesh(mesh);
    }
}

MeshRenderer::MeshRenderer(Mesh *m) : mesh(m), visible(true)
{
}

void MeshRenderer::attach()
{
   // LogInfo("[MeshRenderer] attached to %s", m_owner->getName().c_str());
    
     m_owner->getWorldTransform();

     m_owner->m_boundBox.expand(mesh->GetBoundingBox());

    //  LogInfo("[MeshRenderer] Bounding mesh Box: %f %f %f", mesh->GetBoundingBox().min.x, mesh->GetBoundingBox().min.y, mesh->GetBoundingBox().min.z);
    //  LogInfo("[MeshRenderer] Bounding mesh Box: %f %f %f", mesh->GetBoundingBox().max.x, mesh->GetBoundingBox().max.y, mesh->GetBoundingBox().max.z);


    //  LogInfo("[MeshRenderer] Bounding Box: %f %f %f", m_owner->getBoundingBox().min.x, m_owner->getBoundingBox().min.y, m_owner->getBoundingBox().min.z);
    //  LogInfo("[MeshRenderer] Bounding Box: %f %f %f", m_owner->getBoundingBox().max.x, m_owner->getBoundingBox().max.y, m_owner->getBoundingBox().max.z);
}

void Rotator::update(float deltaTime)
{
    if (!isEnabled())
        return;

    // Rotate GameObject
    GameObject *owner = getOwner();
    if (owner)
    {
        owner->rotateXDeg(rotationSpeed.x * deltaTime);
        owner->rotateYDeg(rotationSpeed.y * deltaTime);
        owner->rotateZDeg(rotationSpeed.z * deltaTime);
    }
}

void Oscillator::start()

{
    // Store initial position
    GameObject *owner = getOwner();
    if (owner)
    {
        startPosition = owner->getPosition();
    }
}
void Oscillator::update(float deltaTime)
{
    if (!isEnabled())
        return;

    time += deltaTime;

    GameObject *owner = getOwner();
    if (owner)
    {
        // Sine wave oscillation
        float wave = std::sin(time * frequency * 2.0f * 3.14159f);
        Vec3 offset = amplitude * wave;
        owner->setPosition(startPosition + offset);
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
    : Component()
    , m_camera(nullptr)
    , m_moveSpeed(10.0f)
    , m_sprintMultiplier(2.0f)
    , m_slowMultiplier(0.25f)
    , m_mouseSensitivity(0.1f)
    , m_pitch(0.0f)
    , m_yaw(0.0f)
    , m_invertY(false)
    , m_minPitch(-89.0f)
    , m_maxPitch(89.0f)
    , m_moveInput(0, 0, 0)
    , m_rotationInput(0, 0)
    , m_isSprinting(false)
    , m_isSlowMode(false)
{
}

void FreeCameraComponent::attach()
{
    // Tentar obter Camera do GameObject
    m_camera = dynamic_cast<Camera*>( getOwner());
    
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
   //Quat rotation = Quat::FromEulerAnglesDeg(m_yaw, 0.0, m_pitch);
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
    //Vec3 up = Vec3(0, 1, 0);  // Up sempre no mundo para 6DOF puro
    
    // Para 6DOF verdadeiro,  o up da câmera:
    Vec3 up = m_camera->getUp();
    
    Vec3 movement = (right * input.x + up * input.y + forward * input.z) * speed * dt;
    
    m_camera->translate(movement, TransformSpace::World);
}

void FreeCameraComponent::setMoveInput(const Vec3& input)
{
    m_moveInput = input;
}

void FreeCameraComponent::setRotationInput(const Vec2& delta)
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

TerrainRenderer::TerrainRenderer(Terrain *t)
{
    terrain = t;
    visible = true;
}

void TerrainRenderer::attach()
{
     m_owner->getWorldTransform();
     m_owner->m_boundBox.merge(terrain->GetBoundingBox());
}

void TerrainRenderer::render()
{
   
    if (visible)
    {
        terrain->Render();
    }
   
}
