#include "pch.h"
#include "Camera.hpp"
#include "Ray.hpp"

Camera3D::Camera3D(const std::string &name)
    : GameObject(name),
      fov(45.0f),
      aspectRatio(16.0f / 9.0f),
      nearPlane(0.1f),
      farPlane(100.0f),
      viewDirty(true),
      projectionDirty(true)
{
}

Camera3D::Camera3D(float fov, float aspect, float near, float far, const std::string &name)
    : GameObject(name),
      fov(fov),
      aspectRatio(aspect),
      nearPlane(near),
      farPlane(far),
      viewDirty(true),
      projectionDirty(true)
{
}

// ==================== Protected Methods ====================

void Camera3D::updateViewMatrix()
{
    if (!viewDirty)
        return;

    viewMatrix = getWorldMatrix().inverse();
    viewDirty = false;
}

void Camera3D::updateProjectionMatrix()
{
    if (!projectionDirty)
        return;

    projectionMatrix = Mat4::PerspectiveDeg(fov, aspectRatio, nearPlane, farPlane);
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

Ray Camera3D::screenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight)
{
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / screenHeight;

    Vec4 rayClip(x, y, -1.0f, 1.0f);

    Mat4 invProj = getProjectionMatrix().inverse();
    Vec4 rayEye = invProj * rayClip;
    rayEye = Vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    Mat4 invView = getViewMatrix().inverse();
    Vec4 rayWorld = invView * rayEye;
    Vec3 direction = Vec3(rayWorld.x, rayWorld.y, rayWorld.z).normalized();

    return Ray(getPosition(), direction);
}
 
// ==================== Utilities ====================

Vec3 Camera3D::getTarget()
{
    Vec3 localForward = Vec3(0, 0, -1);
    return getLocalPosition() + getLocalRotation() * localForward;
   // return getPosition() + forward() * farPlane;
}

void Camera3D::update(float deltaTime)
{
    if (transformDirty)
    {
        viewDirty = true;
    }
}

CameraFPS::CameraFPS(const std::string &name)
    : Camera3D(name),
      pitch(0.0f),
      yaw(0.0f),
      pitchLimit(89.0f),
      moveSpeed(5.0f),
      mouseSensitivity(0.1f)
{
}

CameraFPS::CameraFPS(float fov, float aspect, float near, float far, const std::string &name)
    : Camera3D(fov, aspect, near, far, name),
      pitch(0.0f),
      yaw(0.0f),
      pitchLimit(89.0f),
      moveSpeed(5.0f),
      mouseSensitivity(0.1f)
{
}

// ==================== Movement ====================

void CameraFPS::move(float distance)
{
    Vec3 forward = this->forward();
    forward.y = 0;
    forward = forward.normalized();
    setLocalPosition(getLocalPosition() + forward * distance);
    markViewDirty();
}

void CameraFPS::strafe(float distance)
{
    Vec3 right = this->right();
    right.y = 0;
    right = right.normalized();
    setLocalPosition(getLocalPosition() + right * distance);
    markViewDirty();
}

void CameraFPS::moveUp(float distance)
{
    setLocalPosition(getLocalPosition() + Vec3(0, distance, 0));
    markViewDirty();
}

void CameraFPS::moveDown(float distance)
{
    moveUp(-distance);
}

// ==================== Rotation ====================

void CameraFPS::rotate(float mouseDeltaX, float mouseDeltaY)
{
    yaw += mouseDeltaX * mouseSensitivity;
    pitch -= mouseDeltaY * mouseSensitivity;

    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;

    while (yaw > 360.0f)
        yaw -= 360.0f;
    while (yaw < 0.0f)
        yaw += 360.0f;

    setLocalRotation(pitch, yaw, 0);
    markViewDirty();
}

void CameraFPS::setPitch(float degrees)
{
    pitch = degrees;
    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;

    setLocalRotation(pitch, yaw, 0);
    markViewDirty();
}

void CameraFPS::setYaw(float degrees)
{
    yaw = degrees;
    while (yaw > 360.0f)
        yaw -= 360.0f;
    while (yaw < 0.0f)
        yaw += 360.0f;

    setLocalRotation(pitch, yaw, 0);
    markViewDirty();
}

float CameraFPS::getLookPitch() const
{
    return pitch;
}

float CameraFPS::getLookYaw() const
{
    return yaw;
}

// ==================== Settings ====================

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

float CameraFPS::getPitchLimit() const
{
    return pitchLimit;
}

// ==================== Update ====================

void CameraFPS::update(float deltaTime)
{
    Camera3D::update(deltaTime);
}

CameraFree::CameraFree(const std::string &name)
    : Camera3D(name),
      rotation(Quat::Identity()),
      rollAngle(0.0f),
      moveSpeed(10.0f),
      mouseSensitivity(0.15f),
      rollSpeed(45.0f)
{
}

CameraFree::CameraFree(float fov, float aspect, float near, float far, const std::string &name)
    : Camera3D(fov, aspect, near, far, name),
      rotation(Quat::Identity()),
      rollAngle(0.0f),
      moveSpeed(10.0f),
      mouseSensitivity(0.15f),
      rollSpeed(45.0f)
{
}

// ==================== Movement ====================

void CameraFree::move(float distance)
{
    Vec3 forward = this->forward();
    setLocalPosition(getLocalPosition() + forward * distance);
    markViewDirty();
}

void CameraFree::strafe(float distance)
{
    Vec3 right = this->right();
    setLocalPosition(getLocalPosition() + right * distance);
    markViewDirty();
}

void CameraFree::moveUp(float distance)
{
    Vec3 up = this->up();
    setLocalPosition(getLocalPosition() + up * distance);
    markViewDirty();
}

void CameraFree::moveDown(float distance)
{
    moveUp(-distance);
}

// ==================== Rotation ====================

void CameraFree::rotate(float pitchDelta, float yawDelta)
{
    Quat pitch = Quat::RotationXDeg(pitchDelta * mouseSensitivity);
    Quat yaw = Quat::RotationYDeg(-yawDelta * mouseSensitivity);

    rotation = yaw * rotation * pitch;
    rotation.normalize();

    setLocalRotation(rotation);
    markViewDirty();
}

void CameraFree::roll(float degrees)
{
    rollAngle += degrees;

    Quat rollQuat = Quat::RotationZDeg(rollAngle);
    rotation = rotation * rollQuat;
    rotation.normalize();

    setLocalRotation(rotation);
    markViewDirty();
}

void CameraFree::setRotation(float pitch, float yaw, float roll)
{
    rollAngle = roll;
    rotation = Quat::FromEulerAnglesDeg(pitch, yaw, roll);
    setLocalRotation(rotation);
    markViewDirty();
}

void CameraFree::resetOrientation()
{
    rotation = Quat::Identity();
    rollAngle = 0.0f;
    setLocalRotation(rotation);
    markViewDirty();
}

// ==================== Settings ====================

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

float CameraFree::getMoveSpeed() const
{
    return moveSpeed;
}

float CameraFree::getMouseSensitivity() const
{
    return mouseSensitivity;
}

float CameraFree::getRollSpeed() const
{
    return rollSpeed;
}

// ==================== Update ====================

void CameraFree::update(float deltaTime)
{
    Camera3D::update(deltaTime);
}

CameraOrbit::CameraOrbit(const std::string &name)
    : Camera3D(name),
      target(0, 0, 0),
      distance(10.0f),
      azimuth(45.0f),
      elevation(30.0f),
      minDistance(1.0f),
      maxDistance(100.0f),
      minElevation(-89.0f),
      maxElevation(89.0f),
      orbitSpeed(0.5f),
      panSpeed(0.01f),
      zoomSpeed(1.0f)
{
    updatePosition();
}

CameraOrbit::CameraOrbit(float fov, float aspect, float near, float far, const std::string &name)
    : Camera3D(fov, aspect, near, far, name),
      target(0, 0, 0),
      distance(10.0f),
      azimuth(45.0f),
      elevation(30.0f),
      minDistance(1.0f),
      maxDistance(100.0f),
      minElevation(-89.0f),
      maxElevation(89.0f),
      orbitSpeed(0.5f),
      panSpeed(0.01f),
      zoomSpeed(1.0f)
{
    updatePosition();
}

// ==================== Private Methods ====================

void CameraOrbit::updatePosition()
{
    float azimuthRad = ToRadians(azimuth);
    float elevationRad = ToRadians(elevation);

    float x = target.x + distance * std::cos(elevationRad) * std::sin(azimuthRad);
    float y = target.y + distance * std::sin(elevationRad);
    float z = target.z + distance * std::cos(elevationRad) * std::cos(azimuthRad);

    setLocalPosition(x, y, z);
    lookAt(target, Vec3(0, 1, 0));
    markViewDirty();
}

// ==================== Controls ====================

void CameraOrbit::orbit(float azimuthDelta, float elevationDelta)
{
    azimuth += azimuthDelta * orbitSpeed;
    elevation += elevationDelta * orbitSpeed;

    while (azimuth > 360.0f)
        azimuth -= 360.0f;
    while (azimuth < 0.0f)
        azimuth += 360.0f;

    if (elevation > maxElevation)
        elevation = maxElevation;
    if (elevation < minElevation)
        elevation = minElevation;

    updatePosition();
}

void CameraOrbit::pan(float rightDelta, float upDelta)
{
    Vec3 right = this->right() * rightDelta * panSpeed * distance;
    Vec3 up = this->up() * upDelta * panSpeed * distance;

    target = target + right + up;
    updatePosition();
}

void CameraOrbit::zoom(float delta)
{
    distance -= delta * zoomSpeed;

    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;

    updatePosition();
}

// ==================== Target ====================

void CameraOrbit::setTarget(const Vec3 &target)
{
    this->target = target;
    updatePosition();
}

Vec3 CameraOrbit::getTarget() const
{
    return target;
}

// ==================== Distance ====================

void CameraOrbit::setDistance(float distance)
{
    this->distance = distance;
    if (this->distance < minDistance)
        this->distance = minDistance;
    if (this->distance > maxDistance)
        this->distance = maxDistance;
    updatePosition();
}

float CameraOrbit::getDistance() const
{
    return distance;
}

void CameraOrbit::setDistanceLimits(float min, float max)
{
    minDistance = min;
    maxDistance = max;
    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;
    updatePosition();
}

// ==================== Angles ====================

void CameraOrbit::setAzimuth(float degrees)
{
    azimuth = degrees;
    while (azimuth > 360.0f)
        azimuth -= 360.0f;
    while (azimuth < 0.0f)
        azimuth += 360.0f;
    updatePosition();
}

void CameraOrbit::setElevation(float degrees)
{
    elevation = degrees;
    if (elevation > maxElevation)
        elevation = maxElevation;
    if (elevation < minElevation)
        elevation = minElevation;
    updatePosition();
}

float CameraOrbit::getAzimuth() const
{
    return azimuth;
}

float CameraOrbit::getElevation() const
{
    return elevation;
}

void CameraOrbit::setElevationLimits(float min, float max)
{
    minElevation = min;
    maxElevation = max;
    if (elevation < minElevation)
        elevation = minElevation;
    if (elevation > maxElevation)
        elevation = maxElevation;
    updatePosition();
}

// ==================== Settings ====================

void CameraOrbit::setOrbitSpeed(float speed)
{
    orbitSpeed = speed;
}

void CameraOrbit::setPanSpeed(float speed)
{
    panSpeed = speed;
}

void CameraOrbit::setZoomSpeed(float speed)
{
    zoomSpeed = speed;
}

float CameraOrbit::getOrbitSpeed() const
{
    return orbitSpeed;
}

float CameraOrbit::getPanSpeed() const
{
    return panSpeed;
}

float CameraOrbit::getZoomSpeed() const
{
    return zoomSpeed;
}

// ==================== Utilities ====================

void CameraOrbit::frameObject(const Vec3 &center, float radius)
{
    target = center;

    float fovRad = ToRadians(fov);
    distance = radius / std::tan(fovRad * 0.5f) * 1.5f;

    if (distance < minDistance)
        distance = minDistance;
    if (distance > maxDistance)
        distance = maxDistance;

    updatePosition();
}

void CameraOrbit::resetView()
{
    azimuth = 45.0f;
    elevation = 30.0f;
    distance = 10.0f;
    target = Vec3(0, 0, 0);
    updatePosition();
}

// ==================== Update ====================

void CameraOrbit::update(float deltaTime)
{
    Camera3D::update(deltaTime);
}

CameraMaya::CameraMaya(const std::string &name)
    : Camera3D(name),
      m_target(0, 0, 0),
      m_distance(10.0f),
      m_azimuth(45.0f),
      m_elevation(30.0f),
      m_minDistance(0.1f),
      m_maxDistance(1000.0f),
      m_minElevation(-89.0f),
      m_maxElevation(89.0f),
      m_orbitSensitivity(0.3f),
      m_panSensitivity(0.01f),
      m_zoomSensitivity(0.1f),
      m_smoothing(false),
      m_smoothFactor(10.0f),
      m_targetSmooth(0, 0, 0),
      m_distanceSmooth(10.0f),
      m_azimuthSmooth(45.0f),
      m_elevationSmooth(30.0f)
{
    updateCameraPosition();
}

CameraMaya::CameraMaya(float fov, float aspect, float near, float far, const std::string &name)
    : Camera3D(fov, aspect, near, far, name),
      m_target(0, 0, 0),
      m_distance(10.0f),
      m_azimuth(45.0f),
      m_elevation(30.0f),
      m_minDistance(0.1f),
      m_maxDistance(1000.0f),
      m_minElevation(-89.0f),
      m_maxElevation(89.0f),
      m_orbitSensitivity(0.3f),
      m_panSensitivity(0.01f),
      m_zoomSensitivity(0.1f),
      m_smoothing(false),
      m_smoothFactor(10.0f),
      m_targetSmooth(0, 0, 0),
      m_distanceSmooth(10.0f),
      m_azimuthSmooth(45.0f),
      m_elevationSmooth(30.0f)
{
    updateCameraPosition();
}

// ==================== Controls ====================

void CameraMaya::Orbit(float deltaX, float deltaY)
{
    m_azimuth += deltaX * m_orbitSensitivity;
    m_elevation -= deltaY * m_orbitSensitivity;

    clampAngles();

    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::Pan(float deltaX, float deltaY)
{
    // Pan in screen space (right and up vectors)
    Vec3 right = this->right();
    Vec3 up = this->up();

    // Scale pan by distance (further away = bigger movements)
    float panScale = m_distance * m_panSensitivity;

    Vec3 offset = right * (-deltaX * panScale) + up * (deltaY * panScale);
    m_target = m_target + offset;

    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::Zoom(float delta)
{
    // Zoom by changing distance (negative delta = zoom in)
    m_distance += delta * m_zoomSensitivity * m_distance * 0.1f;
    clampDistance();

    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::ZoomScroll(float scrollDelta)
{
    // Scroll wheel zoom (more intuitive)
    // Negative scroll = zoom in, positive = zoom out
    float zoomFactor = 1.0f - (scrollDelta * m_zoomSensitivity);
    m_distance *= zoomFactor;
    clampDistance();

    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::Dolly(float delta)
{
    // Dolly: move camera and target together along view direction
    Vec3 forward = getViewDirection();
    Vec3 movement = forward * delta * m_zoomSensitivity * 10.0f;

    m_target = m_target + movement;

    if (!m_smoothing)
        updateCameraPosition();
}

// ==================== Focus & Framing ====================

void CameraMaya::FocusOn(const Vec3 &point, float distance)
{
    m_target = point;

    if (distance > 0.0f)
    {
        m_distance = distance;
        clampDistance();
    }

    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::Frame(const Vec3 &center, float radius)
{
    m_target = center;

    // Calculate distance needed to fit sphere in view
    // Using vertical FOV for calculation
    float fovRad = ToRadians(fov);
    float distance = radius / std::tan(fovRad * 0.5f);

    // Add margin (1.5x for comfortable framing)
    m_distance = distance * 1.5f;
    clampDistance();

    if (!m_smoothing)
        updateCameraPosition();
}

// ==================== Target & Distance ====================

void CameraMaya::setTarget(const Vec3 &target)
{
    m_target = target;
    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::setDistance(float distance)
{
    m_distance = distance;
    clampDistance();
    if (!m_smoothing)
        updateCameraPosition();
}

// ==================== Angles ====================

void CameraMaya::setAzimuth(float azimuth)
{
    m_azimuth = azimuth;
    clampAngles();
    if (!m_smoothing)
        updateCameraPosition();
}

void CameraMaya::setElevation(float elevation)
{
    m_elevation = elevation;
    clampAngles();
    if (!m_smoothing)
        updateCameraPosition();
}

// ==================== Constraints ====================

void CameraMaya::setDistanceConstraints(float minDist, float maxDist)
{
    m_minDistance = minDist;
    m_maxDistance = maxDist;
    clampDistance();
}

void CameraMaya::setElevationConstraints(float minElev, float maxElev)
{
    m_minElevation = minElev;
    m_maxElevation = maxElev;
    clampAngles();
}

// ==================== Smoothing ====================

void CameraMaya::setSmoothing(bool enabled, float factor)
{
    m_smoothing = enabled;
    m_smoothFactor = factor;

    if (enabled)
    {
        // Initialize smooth values to current values
        m_targetSmooth = m_target;
        m_distanceSmooth = m_distance;
        m_azimuthSmooth = m_azimuth;
        m_elevationSmooth = m_elevation;
    }
}

// ==================== View Presets ====================

void CameraMaya::ViewFront()
{
    setView(0.0f, 0.0f); // Looking from +Z
}

void CameraMaya::ViewBack()
{
    setView(180.0f, 0.0f); // Looking from -Z
}

void CameraMaya::ViewTop()
{
    setView(0.0f, 89.0f); // Looking down from +Y
}

void CameraMaya::ViewBottom()
{
    setView(0.0f, -89.0f); // Looking up from -Y
}

void CameraMaya::ViewLeft()
{
    setView(90.0f, 0.0f); // Looking from -X
}

void CameraMaya::ViewRight()
{
    setView(-90.0f, 0.0f); // Looking from +X
}

void CameraMaya::ViewIsometric()
{
    setView(45.0f, 35.264f); // Classic isometric angle (arctan(1/sqrt(2)))
}

void CameraMaya::Reset()
{
    m_target = Vec3(0, 0, 0);
    m_distance = 10.0f;
    m_azimuth = 45.0f;
    m_elevation = 30.0f;

    if (m_smoothing)
    {
        m_targetSmooth = m_target;
        m_distanceSmooth = m_distance;
        m_azimuthSmooth = m_azimuth;
        m_elevationSmooth = m_elevation;
    }

    updateCameraPosition();
}

// ==================== Utilities ====================

void CameraMaya::SnapToTarget()
{
    if (m_smoothing)
    {
        m_targetSmooth = m_target;
        m_distanceSmooth = m_distance;
        m_azimuthSmooth = m_azimuth;
        m_elevationSmooth = m_elevation;
    }
    updateCameraPosition();
}

Vec3 CameraMaya::getViewDirection()
{
    return (m_target - getPosition()).normalized();
}

float CameraMaya::getActualDistance()
{
    return (getPosition() - m_target).length();
}

// ==================== Update ====================

void CameraMaya::update(float deltaTime)
{
    Camera3D::update(deltaTime);

    if (m_smoothing)
    {
        updateCameraPositionSmooth(deltaTime);
    }
}

// ==================== Private Methods ====================

void CameraMaya::updateCameraPosition()
{
    // Convert spherical coordinates to cartesian
    float azimuthRad = ToRadians(m_azimuth);
    float elevationRad = ToRadians(m_elevation);

    // Calculate position relative to target
    float x = m_distance * std::cos(elevationRad) * std::sin(azimuthRad);
    float y = m_distance * std::sin(elevationRad);
    float z = m_distance * std::cos(elevationRad) * std::cos(azimuthRad);

    Vec3 offset(x, y, z);
    Vec3 position = m_target + offset;

    // Set camera position and look at target
    setLocalPosition(position);
    lookAt(m_target, Vec3(0, 1, 0));

    markViewDirty();
}

void CameraMaya::updateCameraPositionSmooth(float deltaTime)
{
    // Smooth interpolation factor
    float t = std::min(1.0f, m_smoothFactor * deltaTime);

    // Interpolate target
    m_targetSmooth = Vec3::Lerp(m_targetSmooth, m_target, t);

    // Interpolate distance
    m_distanceSmooth = m_distanceSmooth + (m_distance - m_distanceSmooth) * t;

    // Interpolate angles (with wrap-around handling for azimuth)
    float azimuthDiff = m_azimuth - m_azimuthSmooth;

    // Normalize angle difference to [-180, 180]
    while (azimuthDiff > 180.0f)
        azimuthDiff -= 360.0f;
    while (azimuthDiff < -180.0f)
        azimuthDiff += 360.0f;

    m_azimuthSmooth = m_azimuthSmooth + azimuthDiff * t;
    m_elevationSmooth = m_elevationSmooth + (m_elevation - m_elevationSmooth) * t;

    // Normalize azimuth smooth
    while (m_azimuthSmooth > 360.0f)
        m_azimuthSmooth -= 360.0f;
    while (m_azimuthSmooth < 0.0f)
        m_azimuthSmooth += 360.0f;

    // Convert smooth spherical coordinates to cartesian
    float azimuthRad = ToRadians(m_azimuthSmooth);
    float elevationRad = ToRadians(m_elevationSmooth);

    float x = m_distanceSmooth * std::cos(elevationRad) * std::sin(azimuthRad);
    float y = m_distanceSmooth * std::sin(elevationRad);
    float z = m_distanceSmooth * std::cos(elevationRad) * std::cos(azimuthRad);

    Vec3 offset(x, y, z);
    Vec3 position = m_targetSmooth + offset;

    setLocalPosition(position);
    lookAt(m_targetSmooth, Vec3(0, 1, 0));

    markViewDirty();
}

void CameraMaya::clampAngles()
{
    // Normalize azimuth to [0, 360]
    while (m_azimuth > 360.0f)
        m_azimuth -= 360.0f;
    while (m_azimuth < 0.0f)
        m_azimuth += 360.0f;

    // Clamp elevation
    if (m_elevation > m_maxElevation)
        m_elevation = m_maxElevation;
    if (m_elevation < m_minElevation)
        m_elevation = m_minElevation;
}

void CameraMaya::clampDistance()
{
    if (m_distance < m_minDistance)
        m_distance = m_minDistance;
    if (m_distance > m_maxDistance)
        m_distance = m_maxDistance;
}

void CameraMaya::setView(float azimuth, float elevation, float distance)
{
    m_azimuth = azimuth;
    m_elevation = elevation;

    if (distance > 0.0f)
    {
        m_distance = distance;
        clampDistance();
    }

    clampAngles();

    if (m_smoothing)
    {
        // Smoothly transition to new view
        // Don't snap smooth values
    }
    else
    {
        updateCameraPosition();
    }
}

CameraThirdPerson::CameraThirdPerson(const std::string &name)
    : Camera3D(name),
      targetNode(nullptr),
      offset(0, 2, 5),
      lookAtOffset(0, 1, 0),
      distance(5.0f),
      height(2.0f),
      angle(0.0f),
      followRotation(true),
      rotationSpeed(5.0f),
      minDistance(2.0f),
      maxDistance(20.0f),
      minHeight(0.5f),
      maxHeight(10.0f)
{
}

CameraThirdPerson::CameraThirdPerson(float fov, float aspect, float near, float far, const std::string &name)
    : Camera3D(fov, aspect, near, far, name),
      targetNode(nullptr),
      offset(0, 2, 5),
      lookAtOffset(0, 1, 0),
      distance(5.0f),
      height(2.0f),
      angle(0.0f),
      followRotation(true),
      rotationSpeed(5.0f),
      minDistance(2.0f),
      maxDistance(20.0f),
      minHeight(0.5f),
      maxHeight(10.0f)
{
}

// ==================== Target ====================

void CameraThirdPerson::setTarget(Node3D *target)
{
    targetNode = target;
}

Node3D *CameraThirdPerson::getTarget() const
{
    return targetNode;
}

// ==================== Offset ====================

void CameraThirdPerson::setOffset(const Vec3 &offset)
{
    this->offset = offset;
    distance = offset.length();
    height = offset.y;
}

void CameraThirdPerson::setOffset(float distance, float height, float angle)
{
    this->distance = distance;
    this->height = height;
    this->angle = angle;

    float angleRad = ToRadians(angle);
    offset = Vec3(
        distance * std::sin(angleRad),
        height,
        distance * std::cos(angleRad));
}

Vec3 CameraThirdPerson::getOffset() const
{
    return offset;
}

void CameraThirdPerson::setDistance(float distance)
{
    this->distance = distance;
    if (this->distance < minDistance)
        this->distance = minDistance;
    if (this->distance > maxDistance)
        this->distance = maxDistance;

    setOffset(this->distance, height, angle);
}

void CameraThirdPerson::setHeight(float height)
{
    this->height = height;
    if (this->height < minHeight)
        this->height = minHeight;
    if (this->height > maxHeight)
        this->height = maxHeight;

    setOffset(distance, this->height, angle);
}

void CameraThirdPerson::setAngle(float angle)
{
    this->angle = angle;
    setOffset(distance, height, this->angle);
}

float CameraThirdPerson::getDistance() const
{
    return distance;
}

float CameraThirdPerson::getHeight() const
{
    return height;
}

float CameraThirdPerson::getAngle() const
{
    return angle;
}

// ==================== Look At ====================

void CameraThirdPerson::setLookAtOffset(const Vec3 &offset)
{
    lookAtOffset = offset;
}

Vec3 CameraThirdPerson::getLookAtOffset() const
{
    return lookAtOffset;
}

// ==================== Rotation ====================

void CameraThirdPerson::setFollowRotation(bool follow)
{
    followRotation = follow;
}

bool CameraThirdPerson::getFollowRotation() const
{
    return followRotation;
}

void CameraThirdPerson::setRotationSpeed(float speed)
{
    rotationSpeed = speed;
}

float CameraThirdPerson::getRotationSpeed() const
{
    return rotationSpeed;
}

// ==================== Limits ====================

void CameraThirdPerson::setDistanceLimits(float min, float max)
{
    minDistance = min;
    maxDistance = max;
    if (distance < minDistance)
        setDistance(minDistance);
    if (distance > maxDistance)
        setDistance(maxDistance);
}

void CameraThirdPerson::setHeightLimits(float min, float max)
{
    minHeight = min;
    maxHeight = max;
    if (height < minHeight)
        setHeight(minHeight);
    if (height > maxHeight)
        setHeight(maxHeight);
}

// ==================== Update ====================

void CameraThirdPerson::update(float deltaTime)
{
    Camera3D::update(deltaTime);

    if (targetNode)
    {
        updateCameraPosition(deltaTime);
    }
}

void CameraThirdPerson::updateCameraPosition(float deltaTime)
{
    Vec3 desiredPosition = calculateDesiredPosition();

    setPosition(desiredPosition);

    Vec3 targetLookAt = targetNode->getPosition() + lookAtOffset;
    lookAt(targetLookAt);
}

Vec3 CameraThirdPerson::calculateDesiredPosition() const
{
    Vec3 targetPos = targetNode->getPosition();

    if (followRotation)
    {
        Vec3 rotatedOffset = targetNode->getRotation() * offset;
        return targetPos + rotatedOffset;
    }
    else
    {
        return targetPos + offset;
    }
}

CameraThirdPersonSpring::CameraThirdPersonSpring(const std::string &name)
    : CameraThirdPerson(name),
      currentVelocity(0, 0, 0),
      currentPosition(0, 0, 0),
      currentRotation(Quat::Identity()),
      springStiffness(5.0f),
      springDamping(0.5f),
      rotationLagSpeed(3.0f),
      positionLagSpeed(5.0f),
      enableCollision(false),
      collisionRadius(0.5f),
      collisionMargin(0.2f),
      inheritPitch(false),
      inheritYaw(true),
      inheritRoll(false),
      maxLagDistance(10.0f)
{
}

CameraThirdPersonSpring::CameraThirdPersonSpring(float fov, float aspect, float near, float far, const std::string &name)
    : CameraThirdPerson(fov, aspect, near, far, name),
      currentVelocity(0, 0, 0),
      currentPosition(0, 0, 0),
      currentRotation(Quat::Identity()),
      springStiffness(5.0f),
      springDamping(0.5f),
      rotationLagSpeed(3.0f),
      positionLagSpeed(5.0f),
      enableCollision(false),
      collisionRadius(0.5f),
      collisionMargin(0.2f),
      inheritPitch(false),
      inheritYaw(true),
      inheritRoll(false),
      maxLagDistance(10.0f)
{
}

// ==================== Spring Settings ====================

void CameraThirdPersonSpring::setSpringStiffness(float stiffness)
{
    springStiffness = stiffness;
}

void CameraThirdPersonSpring::setSpringDamping(float damping)
{
    springDamping = damping;
}

float CameraThirdPersonSpring::getSpringStiffness() const
{
    return springStiffness;
}

float CameraThirdPersonSpring::getSpringDamping() const
{
    return springDamping;
}

// ==================== Lag Settings ====================

void CameraThirdPersonSpring::setRotationLagSpeed(float speed)
{
    rotationLagSpeed = speed;
}

void CameraThirdPersonSpring::setPositionLagSpeed(float speed)
{
    positionLagSpeed = speed;
}

float CameraThirdPersonSpring::getRotationLagSpeed() const
{
    return rotationLagSpeed;
}

float CameraThirdPersonSpring::getPositionLagSpeed() const
{
    return positionLagSpeed;
}

void CameraThirdPersonSpring::setMaxLagDistance(float distance)
{
    maxLagDistance = distance;
}

float CameraThirdPersonSpring::getMaxLagDistance() const
{
    return maxLagDistance;
}

// ==================== Collision ====================

void CameraThirdPersonSpring::setEnableCollision(bool enable)
{
    enableCollision = enable;
}

void CameraThirdPersonSpring::setCollisionRadius(float radius)
{
    collisionRadius = radius;
}

void CameraThirdPersonSpring::setCollisionMargin(float margin)
{
    collisionMargin = margin;
}

bool CameraThirdPersonSpring::getEnableCollision() const
{
    return enableCollision;
}

float CameraThirdPersonSpring::getCollisionRadius() const
{
    return collisionRadius;
}

float CameraThirdPersonSpring::getCollisionMargin() const
{
    return collisionMargin;
}

// ==================== Rotation Inheritance ====================

void CameraThirdPersonSpring::setInheritPitch(bool inherit)
{
    inheritPitch = inherit;
}

void CameraThirdPersonSpring::setInheritYaw(bool inherit)
{
    inheritYaw = inherit;
}

void CameraThirdPersonSpring::setInheritRoll(bool inherit)
{
    inheritRoll = inherit;
}

bool CameraThirdPersonSpring::getInheritPitch() const
{
    return inheritPitch;
}

bool CameraThirdPersonSpring::getInheritYaw() const
{
    return inheritYaw;
}

bool CameraThirdPersonSpring::getInheritRoll() const
{
    return inheritRoll;
}

// ==================== Utilities ====================

void CameraThirdPersonSpring::snapToTarget()
{
    if (!targetNode)
        return;

    currentPosition = calculateDesiredPosition();
    currentRotation = calculateDesiredRotation();
    currentVelocity = Vec3(0, 0, 0);

    setPosition(currentPosition);
    setRotation(currentRotation);
}

void CameraThirdPersonSpring::reset()
{
    currentVelocity = Vec3(0, 0, 0);
    snapToTarget();
}

// ==================== Update ====================

void CameraThirdPersonSpring::update(float deltaTime)
{
    Camera3D::update(deltaTime);

    if (targetNode)
    {
        updateCameraPosition(deltaTime);
    }
}

void CameraThirdPersonSpring::updateCameraPosition(float deltaTime)
{
    Vec3 desiredPosition = calculateDesiredPosition();
    Quat desiredRotation = calculateDesiredRotation();

    // Apply spring damping to position
    Vec3 newPosition = applySpringDamping(currentPosition, desiredPosition, deltaTime);

    // Check for max lag distance
    Vec3 targetPos = targetNode->getPosition();
    Vec3 toCamera = newPosition - targetPos;
    float lagDistance = toCamera.length();
    if (lagDistance > maxLagDistance)
    {
        newPosition = targetPos + toCamera.normalized() * maxLagDistance;
    }

    // Apply collision if enabled
    if (enableCollision)
    {
        newPosition = checkCollision(newPosition, targetPos);
    }

    currentPosition = newPosition;

    // Apply rotation lag
    currentRotation = applyRotationLag(currentRotation, desiredRotation, deltaTime);

    // Update camera transform
    setPosition(currentPosition);

    // Look at target with offset
    Vec3 targetLookAt = targetPos + lookAtOffset;
    lookAt(targetLookAt);
}

// ==================== Protected Methods ====================

Vec3 CameraThirdPersonSpring::applySpringDamping(const Vec3 &current, const Vec3 &target, float deltaTime)
{
    // Spring-damper system
    Vec3 displacement = target - current;

    // Spring force: F = -k * x
    Vec3 springForce = displacement * springStiffness;

    // Damping force: F = -c * v
    Vec3 dampingForce = currentVelocity * -springDamping;

    // Total acceleration
    Vec3 acceleration = springForce + dampingForce;

    // Update velocity
    currentVelocity = currentVelocity + acceleration * deltaTime;

    // Update position
    Vec3 newPosition = current + currentVelocity * deltaTime;

    // Alternative: simple lerp for position lag
    float t = std::min(1.0f, positionLagSpeed * deltaTime);
    newPosition = Vec3::Lerp(current, target, t);

    return newPosition;
}

Quat CameraThirdPersonSpring::applyRotationLag(const Quat &current, const Quat &target, float deltaTime)
{
    float t = std::min(1.0f, rotationLagSpeed * deltaTime);
    return Quat::Slerp(current, target, t);
}

Vec3 CameraThirdPersonSpring::checkCollision(const Vec3 &desiredPosition, const Vec3 &targetPosition)
{
    // Simple sphere-cast collision check
    // In a real implementation, you would raycast/spherecast against the scene geometry

    Vec3 direction = desiredPosition - targetPosition;
    float desiredDistance = direction.length();

    if (desiredDistance < 0.001f)
        return targetPosition + Vec3(0, collisionRadius + collisionMargin, 0);

    direction = direction.normalized();

    // For now, just ensure minimum distance
    // In a real implementation, raycast from target to desired position
    // and adjust if hit something

    float minDistance = collisionRadius + collisionMargin;
    if (desiredDistance < minDistance)
    {
        return targetPosition + direction * minDistance;
    }

    // TODO: Implement actual raycast/spherecast against scene
    // Example:
    // Ray ray(targetPosition, direction);
    // RaycastHit hit;
    // if (Physics::Raycast(ray, hit, desiredDistance))
    // {
    //     float safeDistance = hit.distance - collisionMargin;
    //     return targetPosition + direction * safeDistance;
    // }

    return desiredPosition;
}

Quat CameraThirdPersonSpring::calculateDesiredRotation() const
{
    if (!followRotation)
        return currentRotation;

    Quat targetRotation = targetNode->getRotation();
    Vec3 targetEuler = targetRotation.toEulerAnglesDeg();

    // Selectively inherit rotation components
    Vec3 desiredEuler = Vec3(0, 0, 0);

    if (inheritPitch)
        desiredEuler.x = targetEuler.x;

    if (inheritYaw)
        desiredEuler.y = targetEuler.y;

    if (inheritRoll)
        desiredEuler.z = targetEuler.z;

    return Quat::FromEulerAnglesDeg(desiredEuler);
}