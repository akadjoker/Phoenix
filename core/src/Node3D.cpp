#include "pch.h"
#include "Node3D.hpp"

Node3D::Node3D(const std::string &name)
    : Node(name),
      localPosition(0, 0, 0),
      localRotation(Quat::Identity()),
      localScale(1, 1, 1),
      worldPosition(0, 0, 0),
      worldRotation(Quat::Identity()),
      worldScale(1, 1, 1),
      transformDirty(true)
{
}

Node3D::Node3D(const Vec3 &position)
    : Node3D()
{
    setLocalPosition(position);
}

Node3D::Node3D(const Vec3 &position, const Quat &rotation)
    : Node3D()
{
    setLocalPosition(position);
    setLocalRotation(rotation);
}

Node3D::Node3D(const Vec3 &position, const Quat &rotation, const Vec3 &scale)
    : Node3D()
{
    setLocalPosition(position);
    setLocalRotation(rotation);
    setLocalScale(scale);
}

// ==================== Private Methods ====================

void Node3D::updateWorldTransform()
{
    if (!transformDirty)
        return;

    Node3D *parent3D = getParent3D();

    if (parent3D)
    {
        parent3D->updateWorldTransform();

        worldPosition = parent3D->transformPoint(localPosition);
        worldRotation = parent3D->getRotation() * localRotation;
        worldScale = parent3D->getScale() * localScale;
    }
    else
    {
        worldPosition = localPosition;
        worldRotation = localRotation;
        worldScale = localScale;
    }

    Mat4 T = Mat4::Translation(worldPosition);
    Mat4 R = worldRotation.toMat4();
    Mat4 S = Mat4::Scale(worldScale);
    worldMatrix = T * R * S;

    transformDirty = false;
}

void Node3D::markTransformDirty()
{
    transformDirty = true;

    for (Node *child : children)
    {
        Node3D *child3D = dynamic_cast<Node3D *>(child);
        if (child3D)
        {
            child3D->markTransformDirty();
        }
    }
}

// ==================== Local Transform ====================

void Node3D::setLocalPosition(const Vec3 &position)
{
    localPosition = position;
    markTransformDirty();
}

void Node3D::setLocalPosition(float x, float y, float z)
{
    setLocalPosition(Vec3(x, y, z));
}

Vec3 Node3D::getLocalPosition() const
{
    return localPosition;
}

void Node3D::setLocalRotation(const Quat &rotation)
{
    localRotation = rotation.normalized();
    markTransformDirty();
}

void Node3D::setLocalRotation(const Vec3 &eulerDegrees)
{
    localRotation = Quat::FromEulerAnglesDeg(eulerDegrees);
    markTransformDirty();
}

void Node3D::setLocalRotation(float pitch, float yaw, float roll)
{
    setLocalRotation(Vec3(pitch, yaw, roll));
}

Quat Node3D::getLocalRotation() const
{
    return localRotation;
}

Vec3 Node3D::getLocalEulerAngles() const
{
    return localRotation.toEulerAnglesDeg();
}

void Node3D::setLocalScale(const Vec3 &scale)
{
    localScale = scale;
    markTransformDirty();
}

void Node3D::setLocalScale(float uniformScale)
{
    setLocalScale(Vec3(uniformScale, uniformScale, uniformScale));
}

void Node3D::setLocalScale(float x, float y, float z)
{
    setLocalScale(Vec3(x, y, z));
}

Vec3 Node3D::getLocalScale() const
{
    return localScale;
}

// ==================== World Transform ====================

void Node3D::setPosition(const Vec3 &position)
{
    Node3D *parent3D = getParent3D();

    if (parent3D)
    {
        localPosition = parent3D->inverseTransformPoint(position);
    }
    else
    {
        localPosition = position;
    }
    markTransformDirty();
}

void Node3D::setPosition(float x, float y, float z)
{
    setPosition(Vec3(x, y, z));
}

Vec3 Node3D::getPosition()
{
    updateWorldTransform();
    return worldPosition;
}

void Node3D::setRotation(const Quat &rotation)
{
    Node3D *parent3D = getParent3D();

    if (parent3D)
    {
        Quat parentRot = parent3D->getRotation();
        localRotation = parentRot.inverse() * rotation;
    }
    else
    {
        localRotation = rotation;
    }
    markTransformDirty();
}

void Node3D::setRotation(const Vec3 &eulerDegrees)
{
    setRotation(Quat::FromEulerAnglesDeg(eulerDegrees));
}

void Node3D::setRotation(float pitch, float yaw, float roll)
{
    setRotation(Vec3(pitch, yaw, roll));
}

Quat Node3D::getRotation()
{
    updateWorldTransform();
    return worldRotation;
}

Vec3 Node3D::getEulerAngles()
{
    return getRotation().toEulerAnglesDeg();
}

void Node3D::setScale(const Vec3 &scale)
{
    Node3D *parent3D = getParent3D();

    if (parent3D)
    {
        Vec3 parentScale = parent3D->getScale();
        localScale = Vec3(
            scale.x / parentScale.x,
            scale.y / parentScale.y,
            scale.z / parentScale.z);
    }
    else
    {
        localScale = scale;
    }
    markTransformDirty();
}

void Node3D::setScale(float uniformScale)
{
    setScale(Vec3(uniformScale, uniformScale, uniformScale));
}

Vec3 Node3D::getScale()
{
    updateWorldTransform();
    return worldScale;
}

// ==================== Matrices ====================

Mat4 Node3D::getLocalMatrix()
{
    Mat4 T = Mat4::Translation(localPosition);
    Mat4 R = localRotation.toMat4();
    Mat4 S = Mat4::Scale(localScale);
    return T * R * S;
}

Mat4 Node3D::getWorldMatrix()
{
    updateWorldTransform();
    return worldMatrix;
}

// ==================== Directions ====================

Vec3 Node3D::forward()
{
    return getRotation() * Vec3(0, 0, -1);
}

Vec3 Node3D::back()
{
    return getRotation() * Vec3(0, 0, 1);
}

Vec3 Node3D::right()
{
    return getRotation() * Vec3(1, 0, 0);
}

Vec3 Node3D::left()
{
    return getRotation() * Vec3(-1, 0, 0);
}

Vec3 Node3D::up()
{
    return getRotation() * Vec3(0, 1, 0);
}

Vec3 Node3D::down()
{
    return getRotation() * Vec3(0, -1, 0);
}

// ==================== Movement ====================

void Node3D::translate(const Vec3 &translation)
{
    setPosition(getPosition() + translation);
}

void Node3D::translate(float x, float y, float z)
{
    translate(Vec3(x, y, z));
}

void Node3D::translateLocal(const Vec3 &translation)
{
    setLocalPosition(localPosition + translation);
}

void Node3D::translateLocal(float x, float y, float z)
{
    translateLocal(Vec3(x, y, z));
}

void Node3D::moveForward(float distance)
{
    translate(forward() * distance);
}

void Node3D::moveBack(float distance)
{
    translate(back() * distance);
}

void Node3D::moveRight(float distance)
{
    translate(right() * distance);
}

void Node3D::moveLeft(float distance)
{
    translate(left() * distance);
}

void Node3D::moveUp(float distance)
{
    translate(up() * distance);
}

void Node3D::moveDown(float distance)
{
    translate(down() * distance);
}

void Node3D::strafe(float rightDist, float upDist)
{
    translate(right() * rightDist + up() * upDist);
}

void Node3D::strafe(float rightDist, float forwardDist, float upDist)
{
    translate(right() * rightDist + forward() * forwardDist + up() * upDist);
}

// ==================== Rotation ====================

void Node3D::rotate(const Quat &rotation)
{
    setRotation(rotation * getRotation());
}

void Node3D::rotate(const Vec3 &axis, float degrees)
{
    rotate(Quat::FromAxisAngleDeg(axis, degrees));
}

void Node3D::rotateX(float degrees)
{
    rotate(Vec3(1, 0, 0), degrees);
}

void Node3D::rotateY(float degrees)
{
    rotate(Vec3(0, 1, 0), degrees);
}

void Node3D::rotateZ(float degrees)
{
    rotate(Vec3(0, 0, 1), degrees);
}

void Node3D::rotateLocal(const Quat &rotation)
{
    setLocalRotation(localRotation * rotation);
}

void Node3D::rotateLocal(const Vec3 &axis, float degrees)
{
    rotateLocal(Quat::FromAxisAngleDeg(axis, degrees));
}

void Node3D::rotateLocalX(float degrees)
{
    rotateLocal(Vec3(1, 0, 0), degrees);
}

void Node3D::rotateLocalY(float degrees)
{
    rotateLocal(Vec3(0, 1, 0), degrees);
}

void Node3D::rotateLocalZ(float degrees)
{
    rotateLocal(Vec3(0, 0, 1), degrees);
}

void Node3D::rotateFPS(float pitch, float yaw)
{
    Quat yawRot = Quat::RotationYDeg(yaw);
    Quat pitchRot = Quat::RotationXDeg(pitch);
    setRotation(yawRot * getRotation() * pitchRot);
}

// ==================== Look At ====================

void Node3D::lookAt(const Vec3 &target)
{
    lookAt(target, Vec3(0, 1, 0));
}

void Node3D::lookAt(const Vec3 &target, const Vec3 &up)
{
    Vec3 direction = (target - getPosition()).normalized();
    lookDirection(direction, up);
}

void Node3D::lookAt(const Node3D &target)
{
    lookAt(target.worldPosition);
}

void Node3D::lookDirection(const Vec3 &direction)
{
    lookDirection(direction, Vec3(0, 1, 0));
}

void Node3D::lookDirection(const Vec3 &direction, const Vec3 &up)
{
    if (direction.lengthSquared() < 1e-6f)
        return;

    Vec3 forward = direction.normalized();
    Vec3 right = Vec3::Cross(up, forward).normalized();
    Vec3 newUp = Vec3::Cross(forward, right);

    Mat3 rotMatrix;
    rotMatrix.m[0] = right.x;
    rotMatrix.m[3] = newUp.x;
    rotMatrix.m[6] = -forward.x;
    rotMatrix.m[1] = right.y;
    rotMatrix.m[4] = newUp.y;
    rotMatrix.m[7] = -forward.y;
    rotMatrix.m[2] = right.z;
    rotMatrix.m[5] = newUp.z;
    rotMatrix.m[8] = -forward.z;

    setRotation(Quat::FromMat3(rotMatrix));
}

// ==================== Utilities ====================

Vec3 Node3D::transformPoint(const Vec3 &localPoint)
{
    updateWorldTransform();
    return worldMatrix * localPoint;
}

Vec3 Node3D::transformDirection(const Vec3 &localDirection)
{
    return getRotation() * localDirection;
}

Vec3 Node3D::inverseTransformPoint(const Vec3 &worldPoint)
{
    updateWorldTransform();
    Mat4 invMatrix = worldMatrix.inverse();
    return invMatrix * worldPoint;
}

Vec3 Node3D::inverseTransformDirection(const Vec3 &worldDirection)
{
    return getRotation().inverse() * worldDirection;
}

void Node3D::reset()
{
    setLocalPosition(Vec3(0, 0, 0));
    setLocalRotation(Quat::Identity());
    setLocalScale(Vec3(1, 1, 1));
}

void Node3D::setParent(Node *newParent)
{
    if (parent == newParent)
        return;

    Vec3 worldPos = getPosition();
    Quat worldRot = getRotation();
    Vec3 worldScl = getScale();

    Node::setParent(newParent);

    Node3D *parent3D = getParent3D();
    if (parent3D)
    {
        localPosition = parent3D->inverseTransformPoint(worldPos);
        localRotation = parent3D->getRotation().inverse() * worldRot;
        Vec3 parentScale = parent3D->getScale();
        localScale = Vec3(
            worldScl.x / parentScale.x,
            worldScl.y / parentScale.y,
            worldScl.z / parentScale.z);
    }
    else
    {
        localPosition = worldPos;
        localRotation = worldRot;
        localScale = worldScl;
    }

    markTransformDirty();
}

Node3D *Node3D::getParent3D() const
{
    return dynamic_cast<Node3D *>(parent);
}


BoundingBox& Node3D::getBoundingBox()
{
    return m_boundingBox;
}

const BoundingBox Node3D::getBoundingBox() const
{
    return m_boundingBox;
}

const BoundingBox Node3D::getTransformedBoundingBox() const
{
    return BoundingBox::Transform(getBoundingBox(),worldMatrix);
}