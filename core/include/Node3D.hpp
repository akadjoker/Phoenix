#pragma once
#include "Config.hpp"
#include "Node.hpp"
#include "Math.hpp"

/**
 * @brief 3D spatial node with transformation capabilities
 *
 * Node3D extends Node with 3D position, rotation, and scale.
 * All 3D objects (cameras, meshes, lights) inherit from Node3D.
 */
class Node3D : public Node
{
protected:
    // Local transform (relative to parent)
    Vec3 localPosition;
    Quat localRotation;
    Vec3 localScale;

    // Cached world transform
    Vec3 worldPosition;
    Quat worldRotation;
    Vec3 worldScale;
    Mat4 worldMatrix;

    bool transformDirty;

    BoundingBox m_boundingBox;

    void updateWorldTransform();
    void markTransformDirty();

public:
    Node3D(const std::string &name = "Node3D");
    Node3D(const Vec3 &position);
    Node3D(const Vec3 &position, const Quat &rotation);
    Node3D(const Vec3 &position, const Quat &rotation, const Vec3 &scale);
    virtual ~Node3D() {}

    virtual ObjectType getType() override { return ObjectType::Node3D; }

    // ==================== Local Transform ====================

    // Position
    void setLocalPosition(const Vec3 &position);
    void setLocalPosition(float x, float y, float z);
    Vec3 getLocalPosition() const;

    // Rotation
    void setLocalRotation(const Quat &rotation);
    void setLocalRotation(const Vec3 &eulerDegrees);
    void setLocalRotation(float pitch, float yaw, float roll);
    Quat getLocalRotation() const;
    Vec3 getLocalEulerAngles() const;

    // Scale
    void setLocalScale(const Vec3 &scale);
    void setLocalScale(float uniformScale);
    void setLocalScale(float x, float y, float z);
    Vec3 getLocalScale() const;

    // ==================== World Transform ====================

    // Position
    void setPosition(const Vec3 &position);
    void setPosition(float x, float y, float z);
    Vec3 getPosition();

    // Rotation
    void setRotation(const Quat &rotation);
    void setRotation(const Vec3 &eulerDegrees);
    void setRotation(float pitch, float yaw, float roll);
    Quat getRotation();
    Vec3 getEulerAngles();

    // Scale
    void setScale(const Vec3 &scale);
    void setScale(float uniformScale);
    Vec3 getScale();

    // ==================== Matrices ====================

    Mat4 getLocalMatrix();
    Mat4 getWorldMatrix();

    // ==================== Directions (World Space) ====================

    Vec3 forward();
    Vec3 back();
    Vec3 right();
    Vec3 left();
    Vec3 up();
    Vec3 down();

    // ==================== Movement ====================

    // Movement in world space
    void translate(const Vec3 &translation);
    void translate(float x, float y, float z);

    // Movement in local space
    void translateLocal(const Vec3 &translation);
    void translateLocal(float x, float y, float z);

    // Shortcuts (local space)
    void moveForward(float distance);
    void moveBack(float distance);
    void moveRight(float distance);
    void moveLeft(float distance);
    void moveUp(float distance);
    void moveDown(float distance);

    // Strafe (lateral movement without rotation)
    void strafe(float right, float up);
    void strafe(float right, float forward, float up);

    // ==================== Rotation ====================


    // Rotation in world space

    void rotate(const Quat &rotation);
    void rotate(const Vec3 &axis, float degrees);
    void rotateX(float degrees);
    void rotateY(float degrees);
    void rotateZ(float degrees);

    // Rotation in local space
    void rotateLocal(const Quat &rotation);
    void rotateLocal(const Vec3 &axis, float degrees);
    void rotateLocalX(float degrees);
    void rotateLocalY(float degrees);
    void rotateLocalZ(float degrees);


    float getLocalPitch() const;
    float getLocalYaw() const;
    float getLocalRoll() const;

    // FPS-style rotation (limited pitch/yaw)
    void rotateFPS(float pitch, float yaw);

    // ==================== Look At ====================

    void lookAt(const Vec3 &target);
    void lookAt(const Vec3 &target, const Vec3 &up);
    void lookAt(const Node3D &target);

    void lookDirection(const Vec3 &direction);
    void lookDirection(const Vec3 &direction, const Vec3 &up);

    // ==================== Utilities ====================

    Vec3 transformPoint(const Vec3 &localPoint);
    Vec3 transformDirection(const Vec3 &localDirection);
    Vec3 inverseTransformPoint(const Vec3 &worldPoint);
    Vec3 inverseTransformDirection(const Vec3 &worldDirection);

    float getPitch();
    float getYaw();
    float getRoll();


     // Direct angle manipulation (add to current angles)
    void addPitch(float degrees);
    void addYaw(float degrees);
    void addRoll(float degrees);
    
    // Direct angle setting (absolute)
    void setPitch(float degrees);
    void setYaw(float degrees);
    void setRoll(float degrees);

    BoundingBox& getBoundingBox();
    const BoundingBox getBoundingBox() const;
    const BoundingBox getTransformedBoundingBox() const;

    void reset();

    // Override setParent to invalidate transforms
    void setParent(Node *newParent) override;

    // Get typed parent
    Node3D *getParent3D() const;
};