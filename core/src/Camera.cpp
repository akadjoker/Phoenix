#include "pch.h"
#include "Camera.hpp"
#include "Input.hpp"
#include "Ray.hpp"

Camera3D::Camera3D(const std::string &name)
    : GameObject(name),
      fov(45.0f),
      aspectRatio(16.0f / 9.0f),
      nearPlane(0.1f),
      farPlane(100.0f),
      viewDirty(true),
      projectionDirty(true),ignortTargetUpdate(false)
{

    Target = Vec3(0, 0, 100);
    UpVector = Vec3(0, 1, 0);
    updateProjectionMatrix();
    updateViewMatrix();
    TargetAndRotationAreBound = false;
}

Camera3D::Camera3D(float fov, float aspect, float near, float far, const std::string &name)
    : GameObject(name),
      fov(fov),
      aspectRatio(aspect),
      nearPlane(near),
      farPlane(far),
      viewDirty(true),
      projectionDirty(true),
      ignortTargetUpdate(false)
{
    UpVector = Vec3(0, 1, 0);
    Target = Vec3(0, 0, 100);
    updateProjectionMatrix();
    updateViewMatrix();
    TargetAndRotationAreBound = false;
}

void Camera3D::setTarget(const Vec3 &pos)
{
    viewDirty=true;
    Target = pos;

    if (TargetAndRotationAreBound)
    {
        const Vec3 toTarget = Target - getAbsolutePosition();
        Node3D::setRotation(toTarget.getHorizontalAngle());
    }
}

void Camera3D::setRotation(const Vec3 &rotation)
{
    viewDirty=true;
    if (TargetAndRotationAreBound)
        Target = getAbsolutePosition() + rotation.rotationToDirection();

    Node3D::setRotation(rotation);
}

const Vec3 &Camera3D::getTarget() const
{
    return Target;
}

void Camera3D::setUpVector(const Vec3 &pos)
{
    viewDirty=true;
    UpVector = pos;
}

const Vec3 &Camera3D::getUpVector() const
{
    return UpVector;
}
// ==================== Protected Methods ====================

void Camera3D::updateViewMatrix()
{
    //if (!viewDirty)
   //     return;

    Vec3 pos = getAbsolutePosition();
    Vec3 up = UpVector;
    up.normalize();
    
    // Verifica se up e forward não são paralelos
    Vec3 forward = (Target - pos);
    forward.normalize();
    
    float dp = Abs(forward.dot(up));
    if (dp > 0.999f)  // Quase paralelos
    {
        // Ajusta up vector
        if (Abs(forward.x) > 0.1f)
            up = Vec3(0, 0, 1);
        else
            up = Vec3(1, 0, 0);
        up.normalize();
    }
 
    viewMatrix.buildCameraLookAtMatrixRH(pos, Target, up);
    
    viewDirty = false;
}

void Camera3D::updateProjectionMatrix()
{
  //  if (!projectionDirty)
 //      return;

    projectionMatrix.buildProjectionMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
    projectionDirty = false;
}

void Camera3D::markViewDirty()
{
    viewDirty = true;
}

// ==================== Projection ====================

void Camera3D::setPerspective(float fov, float aspect, float near, float far)
{
    this->fov = fov;
    this->aspectRatio = aspect;
    this->nearPlane = near;
    this->farPlane = far;
    projectionDirty = true;
}

void Camera3D::setFOV(float fov)
{
    this->fov = fov;
    projectionDirty = true;
}

void Camera3D::setAspectRatio(float aspect)
{
    this->aspectRatio = aspect;
    projectionDirty = true;
}

void Camera3D::setNearPlane(float near)
{
    this->nearPlane = near;
    projectionDirty = true;
}

void Camera3D::setFarPlane(float far)
{
    this->farPlane = far;
    projectionDirty = true;
}

float Camera3D::getFOV() const
{
    return fov;
}

float Camera3D::getAspectRatio() const
{
    return aspectRatio;
}

float Camera3D::getNearPlane() const
{
    return nearPlane;
}

float Camera3D::getFarPlane() const
{
    return farPlane;
}

// ==================== Matrices ====================

Mat4 Camera3D::getViewMatrix()
{
    updateViewMatrix();
    return viewMatrix;
}

Mat4 Camera3D::getProjectionMatrix()
{
    updateProjectionMatrix();
    return projectionMatrix;
}

Mat4 Camera3D::getViewProjectionMatrix()
{
    return getProjectionMatrix() * getViewMatrix();
}

// ==================== Raycasting ====================

// ==================== Utilities ====================

void Camera3D::update(float deltaTime)
{
    updateViewMatrix();
    updateProjectionMatrix();
    if (transformDirty)
    {
        viewDirty = true;
    }
}
 

CameraFree::CameraFree(const std::string &name)
    : Camera3D(name),
      absoluteYaw(0.0f),
      absolutePitch(0.0f),
      moveSpeed(1.0f),
      mouseSensitivity(0.09f),
      maxVerticalAngle(89.0f),  // Importante: < 90!
      noVerticalMovement(false)
{
}

void CameraFree::updateVectors(Vec3& forward, Vec3& right, Vec3& up)
{
    // Calcula forward a partir de yaw/pitch
    float yawRad = ToRadians(absoluteYaw);
    float pitchRad = ToRadians(absolutePitch);
    
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward.normalize();
    
    // Calcula right (perpendicular ao worldUp e forward)
    Vec3 worldUp(0, 1, 0);
    
    // Evita gimbal lock quando olhas direto para cima/baixo
    if (Abs(forward.y) > 0.999f)
    {
        // Usa um vetor alternativo
        Vec3 alternate = (Abs(forward.x) > 0.1f) ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
        right = alternate.cross(forward);
    }
    else
    {
        right = worldUp.cross(forward);
    }
    
    right.normalize();
    
    // Recalcula up para garantir ortogonalidade perfeita
    up = forward.cross(right);
    up.normalize();
}

void CameraFree::processMouseMovement(float deltaX, float deltaY)
{
    absoluteYaw -= deltaX * mouseSensitivity;
    absolutePitch -= deltaY * mouseSensitivity;
    
    // Limita pitch para evitar gimbal lock
    absolutePitch = Clamp(absolutePitch, -maxVerticalAngle, maxVerticalAngle);
    
    // Normaliza yaw
    while (absoluteYaw < 0.0f) absoluteYaw += 360.0f;
    while (absoluteYaw >= 360.0f) absoluteYaw -= 360.0f;
    
    viewDirty = true;
}

void CameraFree::processInput(float deltaTime)
{
    Vec3 pos = getAbsolutePosition();
    
    // Calcula vetores ortogonais
    Vec3 forward, right, up;
    updateVectors(forward, right, up);
    
    // Movimento
    Vec3 velocity(0);
    
    if (noVerticalMovement)
    {
        // FPS style: movimento no plano horizontal
        Vec3 forwardFlat = forward;
        forwardFlat.y = 0.0f;
        if (forwardFlat.length() > 0.001f)
            forwardFlat.normalize();
        else
            forwardFlat = Vec3(0, 0, 1);  // Fallback
        
        Vec3 rightFlat = right;
        rightFlat.y = 0.0f;
        if (rightFlat.length() > 0.001f)
            rightFlat.normalize();
        else
            rightFlat = Vec3(1, 0, 0);  // Fallback
        
        if (Input::IsKeyDown(KEY_W))
            velocity += forwardFlat;
        if (Input::IsKeyDown(KEY_S))
            velocity -= forwardFlat;
        if (Input::IsKeyDown(KEY_D))
            velocity += rightFlat;
        if (Input::IsKeyDown(KEY_A))
            velocity -= rightFlat;
    }
    else
    {
        // Free camera: movimento em todas direções
        if (Input::IsKeyDown(KEY_W))
            velocity += forward;
        if (Input::IsKeyDown(KEY_S))
            velocity -= forward;
        if (Input::IsKeyDown(KEY_D))
            velocity += right;
        if (Input::IsKeyDown(KEY_A))
            velocity -= right;
    }
    
    // Movimento vertical (sempre no eixo Y mundial)
    if (Input::IsKeyDown(KEY_UP))
        velocity.y += 1.0f;
    if (Input::IsKeyDown(KEY_DOWN))
        velocity.y -= 1.0f;
    
    // Aplica movimento
    if (velocity.length() > 0.001f)
    {
        velocity.normalize();
        pos += velocity * moveSpeed * deltaTime;
        setPosition(pos);
    }
    
    if (!ignortTargetUpdate)
    {
    // Atualiza target
        Target = pos + forward;
    }
    
    // Atualiza UpVector para manter orientação correta
    UpVector = up;
    
    viewDirty = true;
}

void CameraFree::update(float deltaTime)
{
    processInput(deltaTime);
    Camera3D::update(deltaTime);
}