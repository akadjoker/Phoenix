#pragma once
#include "Config.hpp"
#include "GameObject.hpp"

class Ray;

/**
 * @brief Base camera class for 3D rendering
 *
 * Camera3D provides view and projection matrices for rendering.
 * Specialized camera types (FPS, Free, Orbit) inherit from this.
 */

enum class ProjectionType
{
    Perspective,
    Orthographic
};

class Camera : public GameObject
{
private:
    Vec3 m_up;

    ProjectionType m_projectionType;

    // Perspective
    float m_fov;
    float m_aspect;

    // Orthographic
    float m_orthoLeft;
    float m_orthoRight;
    float m_orthoBottom;
    float m_orthoTop;

    // Common
    float m_near;
    float m_far;

    mutable Mat4 m_viewMatrix;
    mutable Mat4 m_projectionMatrix;
    mutable bool m_viewDirty;
    mutable bool m_projectionDirty;

    void updateViewMatrix() const;
    void updateProjectionMatrix() const;

public:
    Camera(const std::string &name = "Camera");

    // Up vector
    void setUp(const Vec3 &up);
    Vec3 getUp() const { return m_up; }

    // Target (calculado a partir da rotação)
    void setTarget(const Vec3& target, TransformSpace space = TransformSpace::World);
    Vec3 getTarget(TransformSpace space = TransformSpace::World) const;
 
    Vec3 getTarget() const;
    Vec3 getDirection() const;

    // Projeção
    void setPerspective(float fovDeg, float aspect, float near, float far);
    void setOrthographic(float left, float right, float bottom, float top, float near, float far);

    ProjectionType getProjectionType() const { return m_projectionType; }
    float getFov() const { return m_fov; }
    float getAspect() const { return m_aspect; }
    float getNear() const { return m_near; }
    float getFar() const { return m_far; }

    void setAspectRatio(float value);
    void setFOV(float value);
    void setNearPlane(float value);
    void setFarPlane(float value);

    // Matrizes
    const Mat4 &getViewMatrix() const;
    const Mat4 &getProjectionMatrix() const;
    Mat4 getViewProjectionMatrix() const;

    Ray screenPointToRay(float screenX, float screenY, float viewportWidth, float viewportHeight) const;

    // Override para invalidar view quando transformar
    void setPosition(const Vec3 &pos, TransformSpace space = TransformSpace::Local) override;
    void setPosition(float x, float y, float z, TransformSpace space = TransformSpace::Local) override;
    void setRotation(const Quat &rot, TransformSpace space = TransformSpace::Local) override;
    void setRotation(float x, float y, float z,  TransformSpace space = TransformSpace::Local);
    void translate(const Vec3 &offset, TransformSpace space = TransformSpace::Local) override;
    void rotate(const Quat &rot, TransformSpace space = TransformSpace::Local) override;
    void lookAt(const Vec3 &target, TransformSpace targetSpace = TransformSpace::World,
                const Vec3 &up = Vec3(0, 1, 0)) override;

    void copyFrom(const Camera *other);
};