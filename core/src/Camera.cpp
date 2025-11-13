#include "pch.h"
#include "Camera.hpp"
#include "Ray.hpp"

Camera::Camera(const std::string &name)
    : GameObject(name), m_up(0, 1, 0), m_projectionType(ProjectionType::Perspective), m_fov(60.0f), m_aspect(16.0f / 9.0f), m_orthoLeft(-10.0f), m_orthoRight(10.0f), m_orthoBottom(-10.0f), m_orthoTop(10.0f), m_near(0.1f), m_far(1000.0f), m_viewDirty(true), m_projectionDirty(true)
{
}

void Camera::setUp(const Vec3 &up)
{
    m_up = up;
    m_viewDirty = true;
}

// Camera.cpp
void Camera::setTarget(const Vec3& target, TransformSpace space)
{
    Vec3 worldTarget = target;
    
    // Converter para world space se necessário
    if (space == TransformSpace::Local && m_parent)
    {
        worldTarget = getWorldTransform().TransformPoint(target);
    }
    else if (space == TransformSpace::Parent && m_parent)
    {
        worldTarget = m_parent->getWorldTransform().TransformPoint(target);
    }
    
    // LookAt atualiza a rotação
    lookAt(worldTarget, TransformSpace::World, m_up);
}

Vec3 Camera::getTarget(TransformSpace space) const
{
    Vec3 forward = getForward(TransformSpace::World);
    Vec3 worldTarget = getPosition(TransformSpace::World) + forward * 10.0f; // Distância arbitrária
    
    // Converter de world para o space pedido
    if (space == TransformSpace::Local && m_parent)
    {
        Mat4 parentInverse = m_parent->getWorldTransform().inverse();
        worldTarget = parentInverse.TransformPoint(worldTarget);
    }
    else if (space == TransformSpace::Parent && m_parent)
    {
        Mat4 parentInverse = m_parent->getWorldTransform().inverse();
        worldTarget = parentInverse.TransformPoint(worldTarget);
    }
    
    return worldTarget;
}

 

Vec3 Camera::getTarget() const
{
    Vec3 forward = getForward(TransformSpace::World);
    return getPosition(TransformSpace::World) + forward;
}

Vec3 Camera::getDirection() const
{
    return getForward(TransformSpace::World);
}

void Camera::setPerspective(float fovDeg, float aspect, float near, float far)
{
    m_projectionType = ProjectionType::Perspective;
    m_fov = fovDeg;
    m_aspect = aspect;
    m_near = near;
    m_far = far;
    m_projectionDirty = true;
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float near, float far)
{
    m_projectionType = ProjectionType::Orthographic;
    m_orthoLeft = left;
    m_orthoRight = right;
    m_orthoBottom = bottom;
    m_orthoTop = top;
    m_near = near;
    m_far = far;
    m_projectionDirty = true;
}

void Camera::updateViewMatrix() const
{
    //  Vec3 eye = getPosition(TransformSpace::World);
    //  Vec3 forward = getForward(TransformSpace::World);
    //  Vec3 target = eye + forward;

    //  m_viewMatrix = Mat4::LookAt(eye, target, m_up);
    m_viewMatrix = getWorldTransform().inverse();
    m_viewDirty = false;
}

void Camera::updateProjectionMatrix() const
{
    if (m_projectionType == ProjectionType::Perspective)
    {
        m_projectionMatrix = Mat4::PerspectiveDeg(m_fov, m_aspect, m_near, m_far);
    }
    else
    {
        m_projectionMatrix = Mat4::Ortho(m_orthoLeft, m_orthoRight,
                                         m_orthoBottom, m_orthoTop,
                                         m_near, m_far);
    }
    m_projectionDirty = false;
}

void Camera::setAspectRatio(float value)
{
    m_projectionType = ProjectionType::Perspective;
    m_aspect = value;
    m_projectionDirty = true;
}

void Camera::setFOV(float value)
{
    m_projectionType = ProjectionType::Perspective;
    m_fov = value;
    m_projectionDirty = true;
}

void Camera::setNearPlane(float value)
{
    m_projectionType = ProjectionType::Perspective;
    m_near = value;
    m_projectionDirty = true;
}

void Camera::setFarPlane(float value)
{
    m_projectionType = ProjectionType::Perspective;
    m_far = value;
    m_projectionDirty = true;
}

const Mat4 &Camera::getViewMatrix() const
{
   if (m_viewDirty)
        updateViewMatrix();

    return m_viewMatrix;
}

const Mat4 &Camera::getProjectionMatrix() const
{
    if (m_projectionDirty)
        updateProjectionMatrix();

    return m_projectionMatrix;
}

Mat4 Camera::getViewProjectionMatrix() const
{
    return getProjectionMatrix() * getViewMatrix();
}

// Overrides para invalidar view
void Camera::setPosition(const Vec3 &pos, TransformSpace space)
{
    Node3D::setPosition(pos, space);
    m_viewDirty = true;
}

void Camera::setPosition(float x, float y, float z, TransformSpace space) 
{
    Node3D::setPosition(x,y,z, space);
    m_viewDirty = true;
}

void Camera::setRotation(float x, float y, float z, TransformSpace space)
{

    Quat rot = Quat::FromEulerAnglesDeg(Vec3(x,y,z));
    Node3D::setRotation(rot, space);
    m_viewDirty = true;
}

void Camera::setRotation(const Quat &rot, TransformSpace space)
{
    Node3D::setRotation(rot, space);
    m_viewDirty = true;
}

void Camera::translate(const Vec3 &offset, TransformSpace space)
{
    Node3D::translate(offset, space);
    m_viewDirty = true;
}

void Camera::rotate(const Quat &rot, TransformSpace space)
{
    Node3D::rotate(rot, space);
    m_viewDirty = true;
}

void Camera::lookAt(const Vec3 &target, TransformSpace targetSpace, const Vec3 &up)
{
    Node3D::lookAt(target, targetSpace, up);
    m_up = up;
    m_viewDirty = true;
}

void Camera::copyFrom(const Camera *other)
{
    setPosition(other->getPosition(TransformSpace::World), TransformSpace::World);
    setRotation(other->getRotation(TransformSpace::World), TransformSpace::World);
    m_up = other->m_up;

    m_projectionType = other->m_projectionType;
    m_fov = other->m_fov;
    m_aspect = other->m_aspect;
    m_orthoLeft = other->m_orthoLeft;
    m_orthoRight = other->m_orthoRight;
    m_orthoBottom = other->m_orthoBottom;
    m_orthoTop = other->m_orthoTop;
    m_near = other->m_near;
    m_far = other->m_far;

    m_viewDirty = true;
    m_projectionDirty = true;
}

// void Camera::copyFromWithMirrorY(const Camera* other, float mirrorY)
// {
//     // Posição refletida em Y
//     Vec3 pos = other->getPosition(TransformSpace::World);
//     pos.y = -pos.y + 2.0f * mirrorY;
//     setPosition(pos, TransformSpace::World);

//     // Rotação refletida (inverter pitch e roll)
//     Vec3 euler = other->getEulerAngles();
//     euler.x = -euler.x;  // Pitch
//     euler.z = -euler.z;  // Roll
//     setEulerAngles(euler);

//     m_up = other->m_up;

//     // Copiar projeção
//     m_projectionType = other->m_projectionType;
//     m_fov = other->m_fov;
//     m_aspect = other->m_aspect;
//     m_orthoLeft = other->m_orthoLeft;
//     m_orthoRight = other->m_orthoRight;
//     m_orthoBottom = other->m_orthoBottom;
//     m_orthoTop = other->m_orthoTop;
//     m_near = other->m_near;
//     m_far = other->m_far;

//     m_viewDirty = true;
//     m_projectionDirty = true;
// }
