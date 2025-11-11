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
class Camera3D : public GameObject
{
protected:
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    Vec3 Target;
    Vec3 UpVector;

    bool projectionDirty;
    bool viewDirty;
    bool InputReceiverEnabled;
    bool TargetAndRotationAreBound;
    bool ignortTargetUpdate;
    Mat4 projectionMatrix;
    Mat4 viewMatrix;

    void updateViewMatrix();
    void updateProjectionMatrix();

public:
    Camera3D(const std::string &name = "Camera3D");
    Camera3D(float fov, float aspect, float near, float far, const std::string &name = "Camera3D");
    virtual ~Camera3D() {}

    virtual ObjectType getType() override { return ObjectType::Camera; }

    // ==================== Projection ====================

    void setPerspective(float fov, float aspect, float near, float far);
    void setFOV(float fov);
    void setAspectRatio(float aspect);
    void setNearPlane(float near);
    void setFarPlane(float far);

    float getFOV() const;
    float getAspectRatio() const;
    float getNearPlane() const;
    float getFarPlane() const;

    void setIgnoreTarget(bool state ) { ignortTargetUpdate = state;}

    // ==================== Matrices ====================

    Mat4 getViewMatrix();
    Mat4 getProjectionMatrix();
    Mat4 getViewProjectionMatrix();

    // ==================== Raycasting ====================

    Ray screenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight);

    // ==================== Utilities ====================

 
    virtual void setTarget(const Vec3 &pos);

    virtual void setRotation(const Vec3 &rotation);

    virtual const Vec3 &getTarget() const;

    virtual void setUpVector(const Vec3 &pos);

    virtual const Vec3 &getUpVector() const;

    void update(float deltaTime) override;

protected:
    void markViewDirty();
};

class CameraFree : public Camera3D
{
private:
    float absoluteYaw;
    float absolutePitch;
    float moveSpeed;
    float mouseSensitivity;
    float maxVerticalAngle;
    bool noVerticalMovement;
    
    void updateVectors(Vec3& forward, Vec3& right, Vec3& up);

public:
    CameraFree(const std::string &name = "CameraFree");
    
    void processMouseMovement(float deltaX, float deltaY);
    void processInput(float deltaTime);
    void update(float deltaTime) override;
};

 