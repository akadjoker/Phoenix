#include "pch.h"
#include "Camera.hpp"
#include "Ray.hpp"

Camera::Camera()
    : fov(45.0f), aspectRatio(16.0f / 9.0f), nearPlane(0.1f), farPlane(100.0f), viewDirty(true), projectionDirty(true) {}

Camera::Camera(float fov, float aspect, float near, float far)
    : fov(fov), aspectRatio(aspect), nearPlane(near), farPlane(far), viewDirty(true), projectionDirty(true) {}

void Camera::updateViewMatrix()
{
    if (!viewDirty)
        return;
    viewMatrix = transform.getWorldMatrix().inverse();
    viewDirty = false;
}

void Camera::updateProjectionMatrix()
{
    if (!projectionDirty)
        return;
    projectionMatrix = Mat4::PerspectiveDeg(fov, aspectRatio, nearPlane, farPlane);
    projectionDirty = false;
}

Vec3 Camera::getPosition()  
{
    return transform.getLocalPosition();
}

Vec3 Camera::getForward() 
{
    return transform.getRotation() * Vec3(0, 0, -1);
}

Vec3 Camera::getTarget()
{
    //return transform.getLocalPosition() + (transform.getRotation() * Vec3(0, 0, -1));

      Vec3 localForward = Vec3(0, 0, -1); // Forward sem rotação world
    return transform.getLocalPosition() + transform.getLocalRotation() * localForward;
}
Ray Camera::screenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight)
{
    // Converte coordenadas de ecrã para NDC (-1 a 1)
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / screenHeight;
    
    // Ray em clip space
    Vec4 rayClip(x, y, -1.0f, 1.0f);
    
    // Ray em eye space
    Mat4 invProj = getProjectionMatrix().inverse();
    Vec4 rayEye = invProj * rayClip;
    rayEye = Vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // Ray em world space
    Mat4 invView = getViewMatrix().inverse();
    Vec4 rayWorld = invView * rayEye;
    Vec3 direction = Vec3(rayWorld.x, rayWorld.y, rayWorld.z).normalized();
    
    return Ray(getPosition(), direction);
}


Vec3 Camera::getRight() 
{
    return transform.getRotation() * Vec3(1, 0, 0);
}

Vec3 Camera::getUp() 
{
    return transform.getRotation() * Vec3(0, 1, 0);
}

void Camera::setPosition(const Vec3 &position)
{
    transform.setLocalPosition(position);
    viewDirty = true;
}

void Camera::setPosition(float x, float y, float z)
{
    setPosition(Vec3(x, y, z));
}

void Camera::lookAt(const Vec3 &target)
{
    lookAt(target, Vec3(0, 1, 0));
}

void Camera::lookAt(const Vec3 &target, const Vec3 &up)
{
    transform.lookAt(target, up);
    viewDirty = true;
}

void Camera::setPerspective(float fovDeg, float aspect, float near, float far)
{
    this->fov = fovDeg;
    this->aspectRatio = aspect;
    this->nearPlane = near;
    this->farPlane = far;
    projectionDirty = true;
}

void Camera::setAspectRatio(float aspect)
{
    this->aspectRatio = aspect;
    projectionDirty = true;
}

void Camera::setFOV(float fovDeg)
{
    this->fov = fovDeg;
    projectionDirty = true;
}

Mat4 Camera::getViewMatrix()
{
    updateViewMatrix();
    return viewMatrix;
}

Mat4 Camera::getProjectionMatrix()
{
    updateProjectionMatrix();
    return projectionMatrix;
}

Mat4 Camera::getViewProjectionMatrix()
{
    return getProjectionMatrix() * getViewMatrix();
}

Transform &Camera::getTransform()
{
    return transform;
}

const Transform &Camera::getTransform() const
{
    return transform;
}

// ==================== Camera FPS ====================

CameraFPS::CameraFPS()
    : Camera(), pitch(0.0f), yaw(0.0f), pitchLimit(89.0f), moveSpeed(5.0f), mouseSensitivity(0.1f) {}

CameraFPS::CameraFPS(float fov, float aspect, float near, float far)
    : Camera(fov, aspect, near, far), pitch(0.0f), yaw(0.0f), pitchLimit(89.0f), moveSpeed(5.0f), mouseSensitivity(0.1f) {}

void CameraFPS::move(float distance)
{
    Vec3 forward = getForward();
    forward.y = 0; // Manter no plano horizontal
    forward = forward.normalized();
    transform.setLocalPosition(transform.getLocalPosition() + forward * distance);
    viewDirty = true;
}

 

 

void CameraFPS::strafe(float distance)
{
    Vec3 right = getRight();
    right.y = 0; // Manter no plano horizontal
    right = right.normalized();
    transform.setLocalPosition(transform.getLocalPosition() + right * distance);
    viewDirty = true;
}

void CameraFPS::moveUp(float distance)
{
    transform.setLocalPosition(transform.getLocalPosition() + Vec3(0, distance, 0));
    viewDirty = true;
}

void CameraFPS::moveDown(float distance)
{
    moveUp(-distance);
}

void CameraFPS::rotate(float mouseDeltaX, float mouseDeltaY)
{
    yaw += mouseDeltaX * mouseSensitivity;
    pitch -= mouseDeltaY * mouseSensitivity;

    // Limitar pitch para evitar gimbal lock
    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;

    // Normalizar yaw
    while (yaw > 360.0f)
        yaw -= 360.0f;
    while (yaw < 0.0f)
        yaw += 360.0f;

    // Atualizar rotação do transform
    transform.setLocalRotation(pitch, yaw, 0);
    viewDirty = true;
}

void CameraFPS::setPitch(float degrees)
{
    pitch = degrees;
    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;
    transform.setLocalRotation(pitch, yaw, 0);
    viewDirty = true;
}

void CameraFPS::setYaw(float degrees)
{
    yaw = degrees;
    while (yaw > 360.0f)
        yaw -= 360.0f;
    while (yaw < 0.0f)
        yaw += 360.0f;
    transform.setLocalRotation(pitch, yaw, 0);
    viewDirty = true;
}

float CameraFPS::getPitch() const
{
    return pitch;
}

float CameraFPS::getYaw() const
{
    return yaw;
}

void CameraFPS::setMoveSpeed(float speed)
{
    moveSpeed = speed;
}

void CameraFPS::setMouseSensitivity(float sensitivity)
{
    mouseSensitivity = sensitivity;
}

void CameraFPS::setPitchLimit(float limit)
{
    pitchLimit = limit;
}

float CameraFPS::getMoveSpeed() const
{
    return moveSpeed;
}

float CameraFPS::getMouseSensitivity() const
{
    return mouseSensitivity;
}

void CameraFPS::update(float deltaTime)
{
    // Placeholder para input automático se necessário
}

// ==================== Camera Free ====================

CameraFree::CameraFree()
    : Camera(), rotation(Quat::Identity()), rollAngle(0.0f), moveSpeed(10.0f), mouseSensitivity(0.15f), rollSpeed(45.0f) {}

CameraFree::CameraFree(float fov, float aspect, float near, float far)
    : Camera(fov, aspect, near, far), rotation(Quat::Identity()), rollAngle(0.0f), moveSpeed(10.0f), mouseSensitivity(0.15f), rollSpeed(45.0f) {}

void CameraFree::move(float distance)
{
    Vec3 forward = getForward();
    transform.setLocalPosition(transform.getLocalPosition() + forward * distance);
    viewDirty = true;
}

 

void CameraFree::strafe(float distance)
{
    Vec3 right = getRight();
    transform.setLocalPosition(transform.getLocalPosition() + right * distance);
    viewDirty = true;
}

void CameraFree::moveUp(float distance)
{
    Vec3 up = getUp();
    transform.setLocalPosition(transform.getLocalPosition() + up * distance);
    viewDirty = true;
}

void CameraFree::moveDown(float distance)
{
    moveUp(-distance);
}

void CameraFree::rotate(float pitchDelta, float yawDelta)
{
    // Rotação livre sem restrições
    Quat pitch = Quat::RotationXDeg(pitchDelta * mouseSensitivity);
    Quat yaw = Quat::RotationYDeg(-yawDelta * mouseSensitivity);

    rotation = yaw * rotation * pitch;
    rotation.normalize();

    transform.setLocalRotation(rotation);
    viewDirty = true;
}

void CameraFree::roll(float degrees)
{
    rollAngle += degrees;

    Quat rollQuat = Quat::RotationZDeg(rollAngle);
    rotation = rotation * rollQuat;
    rotation.normalize();

    transform.setLocalRotation(rotation);
    viewDirty = true;
}

void CameraFree::setRotation(float pitch, float yaw, float roll)
{
    rollAngle = roll;
    rotation = Quat::FromEulerAnglesDeg(pitch, yaw, roll);
    transform.setLocalRotation(rotation);
    viewDirty = true;
}

void CameraFree::resetOrientation()
{
    rotation = Quat::Identity();
    rollAngle = 0.0f;
    transform.setLocalRotation(rotation);
    viewDirty = true;
}

void CameraFree::setMoveSpeed(float speed)
{
    moveSpeed = speed;
}

void CameraFree::setMouseSensitivity(float sensitivity)
{
    mouseSensitivity = sensitivity;
}

void CameraFree::setRollSpeed(float speed)
{
    rollSpeed = speed;
}

void CameraFree::update(float deltaTime)
{
    // Placeholder para input automático se necessário
}

// ==================== Camera Maya ====================

CameraMaya::CameraMaya()
    : Camera(), target(0, 0, 0), distance(10.0f), azimuth(45.0f), elevation(30.0f), minDistance(1.0f), maxDistance(100.0f), minElevation(-89.0f), maxElevation(89.0f), orbitSpeed(0.5f), panSpeed(0.01f), zoomSpeed(1.0f)
{
    updatePosition();
}

CameraMaya::CameraMaya(float fov, float aspect, float near, float far)
    : Camera(fov, aspect, near, far), target(0, 0, 0), distance(10.0f), azimuth(45.0f), elevation(30.0f), minDistance(1.0f), maxDistance(100.0f), minElevation(-89.0f), maxElevation(89.0f), orbitSpeed(0.5f), panSpeed(0.01f), zoomSpeed(1.0f)
{
    updatePosition();
}

void CameraMaya::updatePosition()
{
    // Converter coordenadas esféricas para cartesianas
    float azimuthRad = ToRadians(azimuth);
    float elevationRad = ToRadians(elevation);

    float x = target.x + distance * std::cos(elevationRad) * std::sin(azimuthRad);
    float y = target.y + distance * std::sin(elevationRad);
    float z = target.z + distance * std::cos(elevationRad) * std::cos(azimuthRad);

    transform.setLocalPosition(x, y, z);
    transform.lookAt(target, Vec3(0, 1, 0));
    viewDirty = true;
}

void CameraMaya::orbit(float azimuthDelta, float elevationDelta)
{
    azimuth += azimuthDelta * orbitSpeed;
    elevation += elevationDelta * orbitSpeed;

    // Normalizar azimuth
    while (azimuth > 360.0f)
        azimuth -= 360.0f;
    while (azimuth < 0.0f)
        azimuth += 360.0f;

    // Limitar elevation
    if (elevation > maxElevation)
        elevation = maxElevation;
    if (elevation < minElevation)
        elevation = minElevation;

    updatePosition();
}

void CameraMaya::pan(float rightDelta, float upDelta)
{
    Vec3 right = getRight() * rightDelta * panSpeed * distance;
    Vec3 up = getUp() * upDelta * panSpeed * distance;

    target = target + right + up;
    updatePosition();
}

void CameraMaya::zoom(float delta)
{
    distance -= delta * zoomSpeed;

    // Limitar distância
    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;

    updatePosition();
}

void CameraMaya::setTarget(const Vec3 &newTarget)
{
    target = newTarget;
    updatePosition();
}

void CameraMaya::setDistance(float dist)
{
    distance = dist;
    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;
    updatePosition();
}

void CameraMaya::setAzimuth(float degrees)
{
    azimuth = degrees;
    while (azimuth > 360.0f)
        azimuth -= 360.0f;
    while (azimuth < 0.0f)
        azimuth += 360.0f;
    updatePosition();
}

void CameraMaya::setElevation(float degrees)
{
    elevation = degrees;
    if (elevation > maxElevation)
        elevation = maxElevation;
    if (elevation < minElevation)
        elevation = minElevation;
    updatePosition();
}

Vec3 CameraMaya::getTarget() const
{
    return target;
}

float CameraMaya::getDistance() const
{
    return distance;
}

float CameraMaya::getAzimuth() const
{
    return azimuth;
}

float CameraMaya::getElevation() const
{
    return elevation;
}

void CameraMaya::setDistanceLimits(float min, float max)
{
    minDistance = min;
    maxDistance = max;
    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;
    updatePosition();
}

void CameraMaya::setElevationLimits(float min, float max)
{
    minElevation = min;
    maxElevation = max;
    if (elevation < minElevation)
        elevation = minElevation;
    if (elevation > maxElevation)
        elevation = maxElevation;
    updatePosition();
}

void CameraMaya::setOrbitSpeed(float speed)
{
    orbitSpeed = speed;
}

void CameraMaya::setPanSpeed(float speed)
{
    panSpeed = speed;
}

void CameraMaya::setZoomSpeed(float speed)
{
    zoomSpeed = speed;
}

void CameraMaya::frameObject(const Vec3 &center, float radius)
{
    target = center;

    // Calcular distância para ver o objeto inteiro
    float fovRad = ToRadians(fov);
    distance = radius / std::tan(fovRad * 0.5f) * 1.5f; // 1.5x margin

    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;

    updatePosition();
}

void CameraMaya::resetView()
{
    azimuth = 45.0f;
    elevation = 30.0f;
    distance = 10.0f;
    target = Vec3(0, 0, 0);
    updatePosition();
}

void CameraMaya::update(float deltaTime)
{
}
