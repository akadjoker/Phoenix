#pragma once
#include "Config.hpp"
#include "Node3D.hpp"

class Ray;

/**
 * @brief Base camera class for 3D rendering
 * 
 * Camera3D provides view and projection matrices for rendering.
 * Specialized camera types (FPS, Free, Orbit) inherit from this.
 */
class Camera3D : public Node3D
{
protected:
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    
    bool viewDirty;
    bool projectionDirty;
    
    void updateViewMatrix();
    void updateProjectionMatrix();

public:
    Camera3D(const std::string& name = "Camera3D");
    Camera3D(float fov, float aspect, float near, float far, const std::string& name = "Camera3D");
    virtual ~Camera3D() {}

    virtual ObjectType getType() override {return ObjectType::Camera;}

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

    // ==================== Matrices ====================
    
    Mat4 getViewMatrix();
    Mat4 getProjectionMatrix();
    Mat4 getViewProjectionMatrix();

    // ==================== Raycasting ====================
    
    Ray screenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight);

    // ==================== Utilities ====================
    
    Vec3 getTarget();
    
    void update(float deltaTime) override;

protected:
    void markViewDirty();
};


 

/**
 * @brief First-person shooter style camera
 * 
 * CameraFPS provides FPS-style controls with pitch/yaw rotation
 * and horizontal movement (no vertical look affecting movement).
 */
class CameraFPS : public Camera3D
{
private:
    float pitch;
    float yaw;
    float pitchLimit;
    
    float moveSpeed;
    float mouseSensitivity;

public:
    CameraFPS(const std::string& name = "CameraFPS");
    CameraFPS(float fov, float aspect, float near, float far, const std::string& name = "CameraFPS");

      virtual ObjectType getType() override {return ObjectType::CameraFPS;}

    // ==================== Movement ====================
    
    void move(float distance);
    void strafe(float distance);
    void moveUp(float distance);
    void moveDown(float distance);

    // ==================== Rotation ====================
    
    void rotate(float mouseDeltaX, float mouseDeltaY);
    void setPitch(float degrees);
    void setYaw(float degrees);
    
    float getPitch() const;
    float getYaw() const;

    // ==================== Settings ====================
    
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setPitchLimit(float limit);
    
    float getMoveSpeed() const;
    float getMouseSensitivity() const;
    float getPitchLimit() const;

    // ==================== Update ====================
    
    void update(float deltaTime) override;
};

 

/**
 * @brief Free-flight camera with unrestricted 6DOF movement
 * 
 * CameraFree allows movement and rotation in all directions
 * without restrictions, including roll.
 */
class CameraFree : public Camera3D
{
private:
    Quat rotation;
    float rollAngle;
    
    float moveSpeed;
    float mouseSensitivity;
    float rollSpeed;

public:
    CameraFree(const std::string& name = "CameraFree");
    CameraFree(float fov, float aspect, float near, float far, const std::string& name = "CameraFree");

    virtual ObjectType getType() override {return ObjectType::CameraFree;}

    // ==================== Movement ====================
    
    void move(float distance);
    void strafe(float distance);
    void moveUp(float distance);
    void moveDown(float distance);

    // ==================== Rotation ====================
    
    void rotate(float pitchDelta, float yawDelta);
    void roll(float degrees);
    void setRotation(float pitch, float yaw, float roll);
    void resetOrientation();

    // ==================== Settings ====================
    
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setRollSpeed(float speed);
    
    float getMoveSpeed() const;
    float getMouseSensitivity() const;
    float getRollSpeed() const;

    // ==================== Update ====================
    
    void update(float deltaTime) override;
};

 
/**
 * @brief Orbit camera (Maya/Blender style)
 * 
 * CameraOrbit orbits around a target point with spherical coordinates.
 * Supports orbit, pan, and zoom controls.
 */
class CameraOrbit : public Camera3D
{
private:
    Vec3 target;
    float distance;
    float azimuth;
    float elevation;
    
    float minDistance;
    float maxDistance;
    float minElevation;
    float maxElevation;
    
    float orbitSpeed;
    float panSpeed;
    float zoomSpeed;
    
    void updatePosition();

public:
    CameraOrbit(const std::string& name = "CameraOrbit");
    CameraOrbit(float fov, float aspect, float near, float far, const std::string& name = "CameraOrbit");

    virtual ObjectType getType() override {return ObjectType::CameraOrbit;}

    // ==================== Controls ====================
    
    void orbit(float azimuthDelta, float elevationDelta);
    void pan(float rightDelta, float upDelta);
    void zoom(float delta);

    // ==================== Target ====================
    
    void setTarget(const Vec3& target);
    Vec3 getTarget() const;

    // ==================== Distance ====================
    
    void setDistance(float distance);
    float getDistance() const;
    void setDistanceLimits(float min, float max);

    // ==================== Angles ====================
    
    void setAzimuth(float degrees);
    void setElevation(float degrees);
    float getAzimuth() const;
    float getElevation() const;
    void setElevationLimits(float min, float max);

    // ==================== Settings ====================
    
    void setOrbitSpeed(float speed);
    void setPanSpeed(float speed);
    void setZoomSpeed(float speed);
    
    float getOrbitSpeed() const;
    float getPanSpeed() const;
    float getZoomSpeed() const;

    // ==================== Utilities ====================
    
    void frameObject(const Vec3& center, float radius);
    void resetView();

    // ==================== Update ====================
    
    void update(float deltaTime) override;
};

 

/**
 * @brief   Maya/Blender style orbit camera
 * 
 * CameraMaya provides industry-standard 3D viewport controls:
 * - Orbit: Rotate around target (Alt+LMB)
 * - Pan: Move target (Alt+MMB)
 * - Zoom/Dolly: Change distance (Alt+RMB / Scroll)
 * - View presets (Front, Top, Isometric, etc.)
 * - Optional smoothing for cinematic movement
 */
class CameraMaya : public Camera3D
{
private:
    // Orbit Parameters
    Vec3 m_target;
    float m_distance;
    float m_azimuth;
    float m_elevation;
    
    // Constraints
    float m_minDistance;
    float m_maxDistance;
    float m_minElevation;
    float m_maxElevation;
    
    // Control Sensitivity
    float m_orbitSensitivity;
    float m_panSensitivity;
    float m_zoomSensitivity;
    
    // Smoothing (Optional)
    bool m_smoothing;
    float m_smoothFactor;
    Vec3 m_targetSmooth;
    float m_distanceSmooth;
    float m_azimuthSmooth;
    float m_elevationSmooth;

public:
    explicit CameraMaya(const std::string& name = "CameraMaya");
    CameraMaya(float fov, float aspect, float near, float far, const std::string& name = "CameraMaya");
    virtual ~CameraMaya() {}

    virtual ObjectType getType() override {return ObjectType::CameraMaya;}
    
    // ==================== Controls ====================
    
    /**
     * @brief Orbits camera around target (Alt+LMB drag)
     * @param deltaX Horizontal mouse delta (screen space)
     * @param deltaY Vertical mouse delta (screen space)
     */
    void Orbit(float deltaX, float deltaY);
    
    /**
     * @brief Pans camera and target (Alt+MMB drag)
     * @param deltaX Horizontal mouse delta
     * @param deltaY Vertical mouse delta
     */
    void Pan(float deltaX, float deltaY);
    
    /**
     * @brief Zooms by moving camera toward/away from target (Alt+RMB drag)
     * @param delta Mouse drag delta
     */
    void Zoom(float delta);
    
    /**
     * @brief Zooms using scroll wheel (more natural)
     * @param scrollDelta Scroll wheel delta (-1 or +1 typically)
     */
    void ZoomScroll(float scrollDelta);
    
    /**
     * @brief Dollies camera (moves both camera and target forward/back)
     * Maintains view angle but changes what's in frame
     * @param delta Amount to dolly
     */
    void Dolly(float delta);
    
    // ==================== Focus & Framing ====================
    
    /**
     * @brief Focuses on a specific point (changes target, optionally distance)
     * @param point World space point to focus on
     * @param distance New distance (-1 to keep current)
     */
    void FocusOn(const Vec3& point, float distance = -1.0f);
    
    /**
     * @brief Frames a bounding sphere (like Maya 'F' key)
     * Adjusts distance to fit object in view
     * @param center Center of object
     * @param radius Bounding radius
     */
    void Frame(const Vec3& center, float radius);
    
    // ==================== Target & Distance ====================
    
    const Vec3& getTarget() const { return m_target; }
    void setTarget(const Vec3& target);
    
    float getDistance() const { return m_distance; }
    void setDistance(float distance);
    
    // ==================== Angles ====================
    
    float getAzimuth() const { return m_azimuth; }
    void setAzimuth(float azimuth);
    
    float getElevation() const { return m_elevation; }
    void setElevation(float elevation);
    
    // ==================== Constraints ====================
    
    void setDistanceConstraints(float minDist, float maxDist);
    void setElevationConstraints(float minElev, float maxElev);
    
    float getMinDistance() const { return m_minDistance; }
    float getMaxDistance() const { return m_maxDistance; }
    float getMinElevation() const { return m_minElevation; }
    float getMaxElevation() const { return m_maxElevation; }
    
    // ==================== Sensitivity ====================
    
    void setOrbitSensitivity(float sensitivity) { m_orbitSensitivity = sensitivity; }
    float getOrbitSensitivity() const { return m_orbitSensitivity; }
    
    void setPanSensitivity(float sensitivity) { m_panSensitivity = sensitivity; }
    float getPanSensitivity() const { return m_panSensitivity; }
    
    void setZoomSensitivity(float sensitivity) { m_zoomSensitivity = sensitivity; }
    float getZoomSensitivity() const { return m_zoomSensitivity; }
    
    // ==================== Smoothing ====================
    
    /**
     * @brief Enables/disables smooth camera movement
     * @param enabled Enable smoothing
     * @param factor Smoothing factor (higher = faster response, 0-20 typical)
     */
    void setSmoothing(bool enabled, float factor = 10.0f);
    bool isSmoothing() const { return m_smoothing; }
    float getSmoothFactor() const { return m_smoothFactor; }
    
    // ==================== View Presets ====================
    
    /**
     * @brief Standard orthographic views (like Maya/Blender)
     */
    void ViewFront();      // Look from +Z
    void ViewBack();       // Look from -Z
    void ViewTop();        // Look from +Y (down)
    void ViewBottom();     // Look from -Y (up)
    void ViewLeft();       // Look from -X
    void ViewRight();      // Look from +X
    void ViewIsometric();  // Classic 45-45 isometric
    
    /**
     * @brief Resets to default view
     */
    void Reset();
    
    // ==================== Utilities ====================
    
    /**
     * @brief Snaps to target immediately (disables smooth for one frame)
     */
    void SnapToTarget();
    
    /**
     * @brief Returns camera's forward direction in world space
     */
    Vec3 getViewDirection()  ;
    
    /**
     * @brief Returns distance from camera to target
     */
    float getActualDistance()  ;
    
    // ==================== Update ====================
    
    void update(float deltaTime) override;

private:
    void updateCameraPosition();
    void updateCameraPositionSmooth(float deltaTime);
    void clampAngles();
    void clampDistance();
    
    // View preset helper
    void setView(float azimuth, float elevation, float distance = -1.0f);
}; 

/**
 * @brief Third-person camera that follows a target node
 * 
 * CameraThirdPerson maintains a fixed offset behind a target,
 * useful for character controllers and basic vehicle following.
 */


/*

Node3D* player = new Node3D("Player");
player->setPosition(Vec3(0, 0, 0));

CameraThirdPerson* camera = new CameraThirdPerson();
camera->setTarget(player);
camera->setOffset(5.0f, 2.0f, 0.0f);  // 5m atrás, 2m acima
camera->setLookAtOffset(Vec3(0, 1.5f, 0));  // Olha para cabeça
camera->setFollowRotation(true);
root->addChild(camera);

*/

class CameraThirdPerson : public Camera3D
{
protected:
    Node3D* targetNode;
    
    Vec3 offset;
    Vec3 lookAtOffset;
    
    float distance;
    float height;
    float angle;
    
    bool followRotation;
    float rotationSpeed;
    
    float minDistance;
    float maxDistance;
    float minHeight;
    float maxHeight;

public:
    CameraThirdPerson(const std::string& name = "CameraThirdPerson");
    CameraThirdPerson(float fov, float aspect, float near, float far, const std::string& name = "CameraThirdPerson");
    virtual ~CameraThirdPerson() {}

    virtual ObjectType getType() override {return ObjectType::CameraThirdPerson;}
    // ==================== Target ====================
    
    void setTarget(Node3D* target);
    Node3D* getTarget() const;

    // ==================== Offset ====================
    
    void setOffset(const Vec3& offset);
    void setOffset(float distance, float height, float angle);
    Vec3 getOffset() const;
    
    void setDistance(float distance);
    void setHeight(float height);
    void setAngle(float angle);
    
    float getDistance() const;
    float getHeight() const;
    float getAngle() const;

    // ==================== Look At ====================
    
    void setLookAtOffset(const Vec3& offset);
    Vec3 getLookAtOffset() const;

    // ==================== Rotation ====================
    
    void setFollowRotation(bool follow);
    bool getFollowRotation() const;
    
    void setRotationSpeed(float speed);
    float getRotationSpeed() const;

    // ==================== Limits ====================
    
    void setDistanceLimits(float min, float max);
    void setHeightLimits(float min, float max);

    // ==================== Update ====================
    
    void update(float deltaTime) override;

protected:
    virtual void updateCameraPosition(float deltaTime);
    Vec3 calculateDesiredPosition() const;
};

 
/**
 * @brief Spring arm third-person camera with smooth following and collision
 * 
 * CameraThirdPersonSpring extends third-person camera with:
 * - Smooth spring-based following (damping)
 * - Collision detection (optional raycast to avoid geometry)
 * - Smooth rotation interpolation
 * - Ideal for racing games and vehicles
 */

/*

// Arcade Racing (Forza, Need for Speed)
camera->setSpringStiffness(4.0f);
camera->setSpringDamping(0.7f);
camera->setRotationLagSpeed(3.0f);
camera->setInheritYaw(true);
camera->setInheritPitch(false);

// Simulation Racing (Gran Turismo, Assetto Corsa)
camera->setSpringStiffness(2.0f);
camera->setSpringDamping(0.9f);
camera->setRotationLagSpeed(1.5f);
camera->setInheritYaw(true);
camera->setInheritPitch(true);  // Mais realista

// Action Game (GTA, Uncharted)
camera->setSpringStiffness(5.0f);
camera->setSpringDamping(0.5f);
camera->setRotationLagSpeed(4.0f);
camera->setInheritYaw(true);


Node3D* car = new Node3D("RaceCar");
car->setPosition(Vec3(0, 0, 0));

CameraThirdPersonSpring* camera = new CameraThirdPersonSpring();
camera->setTarget(car);

// Position
camera->setOffset(8.0f, 3.0f, 0.0f);  // 8m atrás, 3m acima
camera->setLookAtOffset(Vec3(0, 0.5f, 0));

// Spring physics (suave como Need for Speed)
camera->setSpringStiffness(3.0f);     // Quão rápido "puxa"
camera->setSpringDamping(0.8f);       // Amortecimento
camera->setPositionLagSpeed(4.0f);    // Lag de posição
camera->setRotationLagSpeed(2.0f);    // Lag de rotação (mais lento)

// Rotation inheritance (importante para carros!)
camera->setInheritYaw(true);          // Segue direção do carro
camera->setInheritPitch(false);       // NÃO segue inclinação (mais estável)
camera->setInheritRoll(false);        // NÃO rola com o carro

// Collision (evita câmara entrar em paredes)
camera->setEnableCollision(true);
camera->setCollisionRadius(0.5f);

// Max lag (previne câmara fugir muito em colisões)
camera->setMaxLagDistance(15.0f);

root->addChild(camera);

// Snap inicial para não começar na origem
camera->snapToTarget();
*/
class CameraThirdPersonSpring : public CameraThirdPerson
{
private:
    Vec3 currentVelocity;
    Vec3 currentPosition;
    Quat currentRotation;
    
    float springStiffness;
    float springDamping;
    
    float rotationLagSpeed;
    float positionLagSpeed;
    
    bool enableCollision;
    float collisionRadius;
    float collisionMargin;
    
    bool inheritPitch;
    bool inheritYaw;
    bool inheritRoll;
    
    float maxLagDistance;

public:
    CameraThirdPersonSpring(const std::string& name = "CameraThirdPersonSpring");
    CameraThirdPersonSpring(float fov, float aspect, float near, float far, const std::string& name = "CameraThirdPersonSpring");

    
    
    virtual ObjectType getType() override {return ObjectType::CameraThirdPersonSpring;}

    // ==================== Spring Settings ====================
    
    void setSpringStiffness(float stiffness);
    void setSpringDamping(float damping);
    
    float getSpringStiffness() const;
    float getSpringDamping() const;

    // ==================== Lag Settings ====================
    
    void setRotationLagSpeed(float speed);
    void setPositionLagSpeed(float speed);
    
    float getRotationLagSpeed() const;
    float getPositionLagSpeed() const;
    
    void setMaxLagDistance(float distance);
    float getMaxLagDistance() const;

    // ==================== Collision ====================
    
    void setEnableCollision(bool enable);
    void setCollisionRadius(float radius);
    void setCollisionMargin(float margin);
    
    bool getEnableCollision() const;
    float getCollisionRadius() const;
    float getCollisionMargin() const;

    // ==================== Rotation Inheritance ====================
    
    void setInheritPitch(bool inherit);
    void setInheritYaw(bool inherit);
    void setInheritRoll(bool inherit);
    
    bool getInheritPitch() const;
    bool getInheritYaw() const;
    bool getInheritRoll() const;

    // ==================== Utilities ====================
    
    void snapToTarget();
    void reset();

    // ==================== Update ====================
    
    void update(float deltaTime) override;

protected:
    void updateCameraPosition(float deltaTime) override;
    
    Vec3 applySpringDamping(const Vec3& current, const Vec3& target, float deltaTime);
    Quat applyRotationLag(const Quat& current, const Quat& target, float deltaTime);
    
    Vec3 checkCollision(const Vec3& desiredPosition, const Vec3& targetPosition);
    
    Quat calculateDesiredRotation() const;
};