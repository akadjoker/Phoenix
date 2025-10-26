#pragma once

#include "Transform.hpp"

class Camera
{
protected:
    Transform transform;

    // Parâmetros da projeção
    float fov; // Field of view (graus)
    float aspectRatio;
    float nearPlane;
    float farPlane;

    // Matrizes cached
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    Mat4 viewProjectionMatrix;
    bool viewDirty;
    bool projectionDirty;

    void updateViewMatrix();
    void updateProjectionMatrix();

public:
    Camera();
    Camera(float fov, float aspect, float near, float far);
    virtual ~Camera() {}

    // Getters do transform
    Vec3 getPosition();
    Vec3 getForward();
    Vec3 getRight();
    Vec3 getUp();

    // Setters básicos
    void setPosition(const Vec3 &position);
    void setPosition(float x, float y, float z);

    // LookAt
    void lookAt(const Vec3 &target);
    void lookAt(const Vec3 &target, const Vec3 &up);

    // Projeção
    void setPerspective(float fov, float aspect, float near, float far);
    void setAspectRatio(float aspect);
    void setFOV(float fov);

    // Matrizes
    Mat4 getViewMatrix();
    Mat4 getProjectionMatrix();
    Mat4 getViewProjectionMatrix();

    // Acesso ao transform
    Transform &getTransform();
    const Transform &getTransform() const;

    // Update (virtual para override)
    virtual void update(float deltaTime) {}
};

// ==================== Camera FPS ====================
class CameraFPS : public Camera
{
private:
    float pitch;      // Rotação X (olhar cima/baixo)
    float yaw;        // Rotação Y (olhar esquerda/direita)
    float pitchLimit; // Limite para evitar gimbal lock

    float moveSpeed;
    float mouseSensitivity;

public:
    CameraFPS();
    CameraFPS(float fov, float aspect, float near, float far);

    // Controles de movimento (WASD)
    void move(float distance);
    void strafe(float distance);
    
    void moveUp(float distance);
    void moveDown(float distance);

    // Controles de mouse
    void rotate(float mouseDeltaX, float mouseDeltaY);
    void setPitch(float degrees);
    void setYaw(float degrees);
    float getPitch() const;
    float getYaw() const;

    // Configurações
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setPitchLimit(float limit);

    float getMoveSpeed() const;
    float getMouseSensitivity() const;

    // Update automático (chama com input)
    void update(float deltaTime) override;
};

// ==================== Camera Free (Fly) ====================
class CameraFree : public Camera
{
private:
    Quat rotation;
    float rollAngle;

    float moveSpeed;
    float mouseSensitivity;
    float rollSpeed;

public:
    CameraFree();
    CameraFree(float fov, float aspect, float near, float far);

    // Movimento em 6 direções (sem restrições)
    void move(float distance);
    void strafe(float distance);
    void moveUp(float distance);
    void moveDown(float distance);

    // Rotação livre (pitch, yaw, roll)
    void rotate(float pitchDelta, float yawDelta);
    void roll(float degrees);
    void setRotation(float pitch, float yaw, float roll);

    // Reset orientação
    void resetOrientation();

    // Configurações
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setRollSpeed(float speed);

    // Update
    void update(float deltaTime) override;
};

// ==================== Camera Maya/Orbit ====================
class CameraMaya : public Camera
{
private:
    Vec3 target;     // Ponto de foco
    float distance;  // Distância do target
    float azimuth;   // Ângulo horizontal (graus)
    float elevation; // Ângulo vertical (graus)

    float minDistance;
    float maxDistance;
    float minElevation;
    float maxElevation;

    float orbitSpeed;
    float panSpeed;
    float zoomSpeed;

    void updatePosition();

public:
    CameraMaya();
    CameraMaya(float fov, float aspect, float near, float far);

    // Controles Maya-style
    void orbit(float azimuthDelta, float elevationDelta); // ALT + Left drag
    void pan(float rightDelta, float upDelta);            // ALT + Middle drag
    void zoom(float delta);                               // ALT + Right drag / Scroll

    // Setters
    void setTarget(const Vec3 &target);
    void setDistance(float distance);
    void setAzimuth(float degrees);
    void setElevation(float degrees);

    // Getters
    Vec3 getTarget() const;
    float getDistance() const;
    float getAzimuth() const;
    float getElevation() const;

    // Limites
    void setDistanceLimits(float min, float max);
    void setElevationLimits(float min, float max);

    // Configurações
    void setOrbitSpeed(float speed);
    void setPanSpeed(float speed);
    void setZoomSpeed(float speed);

    // Frame object (ajusta distância para ver objeto inteiro)
    void frameObject(const Vec3 &center, float radius);

    // Reset para view padrão
    void resetView();

    // Update
    void update(float deltaTime) override;
};
