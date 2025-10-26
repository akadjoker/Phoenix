#include "pch.h"
#include "Transform.hpp"

Transform::Transform()
    : localPosition(0, 0, 0), localRotation(Quat::Identity()), localScale(1, 1, 1), worldPosition(0, 0, 0), worldRotation(Quat::Identity()), worldScale(1, 1, 1), parent(nullptr), localDirty(true), worldDirty(true) {}

Transform::Transform(const Vec3 &position)
    : Transform()
{
    setLocalPosition(position);
}

Transform::Transform(const Vec3 &position, const Quat &rotation)
    : Transform()
{
    setLocalPosition(position);
    setLocalRotation(rotation);
}

Transform::Transform(const Vec3 &position, const Quat &rotation, const Vec3 &scale)
    : Transform()
{
    setLocalPosition(position);
    setLocalRotation(rotation);
    setLocalScale(scale);
}

Transform::~Transform()
{
    // Remover de parent
    if (parent)
    {
        parent->removeChild(this);
    }

    // Remover referências dos filhos
    for (Transform *child : children)
    {
        child->parent = nullptr;
    }
}

// ==================== Private Methods ====================

void Transform::updateWorldTransform()
{
    if (!worldDirty)
        return;

    if (parent)
    {
        // Combinar com transform do parent
        parent->updateWorldTransform(); // Garantir que parent está atualizado

        worldPosition = parent->transformPoint(localPosition);
        worldRotation = parent->getRotation() * localRotation;
        worldScale = parent->getScale() * localScale;
    }
    else
    {
        // Sem parent, world = local
        worldPosition = localPosition;
        worldRotation = localRotation;
        worldScale = localScale;
    }

    // Calcular matriz world
    Mat4 T = Mat4::Translation(worldPosition);
    Mat4 R = worldRotation.toMat4();
    Mat4 S = Mat4::Scale(worldScale);
    worldMatrix = T * R * S;

    worldDirty = false;
}

void Transform::markDirty()
{
    worldDirty = true;

    // Propagar para filhos
    for (Transform *child : children)
    {
        child->markDirty();
    }
}

// ==================== Local Transform ====================

void Transform::setLocalPosition(const Vec3 &position)
{
    localPosition = position;
    markDirty();
}

void Transform::setLocalPosition(float x, float y, float z)
{
    setLocalPosition(Vec3(x, y, z));
}

Vec3 Transform::getLocalPosition() const
{
    return localPosition;
}

void Transform::setLocalRotation(const Quat &rotation)
{
    localRotation = rotation.normalized();
    markDirty();
}

void Transform::setLocalRotation(const Vec3 &eulerDegrees)
{
    localRotation = Quat::FromEulerAnglesDeg(eulerDegrees);
    markDirty();
}

void Transform::setLocalRotation(float pitch, float yaw, float roll)
{
    setLocalRotation(Vec3(pitch, yaw, roll));
}

Quat Transform::getLocalRotation() const
{
    return localRotation;
}

Vec3 Transform::getLocalEulerAngles() const
{
    return localRotation.toEulerAnglesDeg();
}

void Transform::setLocalScale(const Vec3 &scale)
{
    localScale = scale;
    markDirty();
}

void Transform::setLocalScale(float uniformScale)
{
    setLocalScale(Vec3(uniformScale, uniformScale, uniformScale));
}

void Transform::setLocalScale(float x, float y, float z)
{
    setLocalScale(Vec3(x, y, z));
}

Vec3 Transform::getLocalScale() const
{
    return localScale;
}

// ==================== World Transform ====================

void Transform::setPosition(const Vec3 &position)
{
    if (parent)
    {
        // Converter world position para local
        localPosition = parent->inverseTransformPoint(position);
    }
    else
    {
        localPosition = position;
    }
    markDirty();
}

void Transform::setPosition(float x, float y, float z)
{
    setPosition(Vec3(x, y, z));
}

Vec3 Transform::getPosition()
{
    updateWorldTransform();
    return worldPosition;
}

void Transform::setRotation(const Quat &rotation)
{
    if (parent)
    {
        // Converter world rotation para local
        Quat parentRot = parent->getRotation();
        localRotation = parentRot.inverse() * rotation;
    }
    else
    {
        localRotation = rotation;
    }
    markDirty();
}

void Transform::setRotation(const Vec3 &eulerDegrees)
{
    setRotation(Quat::FromEulerAnglesDeg(eulerDegrees));
}

void Transform::setRotation(float pitch, float yaw, float roll)
{
    setRotation(Vec3(pitch, yaw, roll));
}

Quat Transform::getRotation()
{
    updateWorldTransform();
    return worldRotation;
}

Vec3 Transform::getEulerAngles()
{
    return getRotation().toEulerAnglesDeg();
}

void Transform::setScale(const Vec3 &scale)
{
    if (parent)
    {
        Vec3 parentScale = parent->getScale();
        localScale = Vec3(scale.x / parentScale.x,
                          scale.y / parentScale.y,
                          scale.z / parentScale.z);
    }
    else
    {
        localScale = scale;
    }
    markDirty();
}

void Transform::setScale(float uniformScale)
{
    setScale(Vec3(uniformScale, uniformScale, uniformScale));
}

Vec3 Transform::getScale()
{
    updateWorldTransform();
    return worldScale;
}

// ==================== Matrizes ====================

Mat4 Transform::getLocalMatrix()
{
    Mat4 T = Mat4::Translation(localPosition);
    Mat4 R = localRotation.toMat4();
    Mat4 S = Mat4::Scale(localScale);
    return T * R * S;
}

Mat4 Transform::getWorldMatrix()
{
    updateWorldTransform();
    return worldMatrix;
}

// ==================== Hierarquia ====================

void Transform::setParent(Transform *newParent)
{
    if (parent == newParent)
        return;

    // Remover do parent antigo
    if (parent)
    {
        parent->removeChild(this);
    }

    // Manter world transform ao mudar de parent
    Vec3 worldPos = getPosition();
    Quat worldRot = getRotation();
    Vec3 worldScl = getScale();

    parent = newParent;

    // Adicionar ao novo parent
    if (parent)
    {
        parent->addChild(this);

        // Converter world transform para local no novo parent
        localPosition = parent->inverseTransformPoint(worldPos);
        localRotation = parent->getRotation().inverse() * worldRot;
        Vec3 parentScale = parent->getScale();
        localScale = Vec3(worldScl.x / parentScale.x,
                          worldScl.y / parentScale.y,
                          worldScl.z / parentScale.z);
    }
    else
    {
        localPosition = worldPos;
        localRotation = worldRot;
        localScale = worldScl;
    }

    markDirty();
}

Transform *Transform::getParent() const
{
    return parent;
}

void Transform::removeParent()
{
    setParent(nullptr);
}

void Transform::addChild(Transform *child)
{
    if (!child || child == this)
        return;

    // Verificar se já é filho
    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end())
    {
        children.push_back(child);
    }
}

void Transform::removeChild(Transform *child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
    }
}

const std::vector<Transform *> &Transform::getChildren() const
{
    return children;
}

int Transform::getChildCount() const
{
    return static_cast<int>(children.size());
}

Transform *Transform::getChild(int index) const
{
    if (index >= 0 && index < static_cast<int>(children.size()))
    {
        return children[index];
    }
    return nullptr;
}

// ==================== Direções ====================

Vec3 Transform::forward()
{
    return getRotation() * Vec3(0, 0, -1);
}

Vec3 Transform::back()
{
    return getRotation() * Vec3(0, 0, 1);
}

Vec3 Transform::right()
{
    return getRotation() * Vec3(1, 0, 0);
}

Vec3 Transform::left()
{
    return getRotation() * Vec3(-1, 0, 0);
}

Vec3 Transform::up()
{
    return getRotation() * Vec3(0, 1, 0);
}

Vec3 Transform::down()
{
    return getRotation() * Vec3(0, -1, 0);
}

// ==================== Movimento ====================

void Transform::translate(const Vec3 &translation)
{
    setPosition(getPosition() + translation);
}

void Transform::translate(float x, float y, float z)
{
    translate(Vec3(x, y, z));
}

void Transform::translateLocal(const Vec3 &translation)
{
    setLocalPosition(localPosition + translation);
}

void Transform::translateLocal(float x, float y, float z)
{
    translateLocal(Vec3(x, y, z));
}

void Transform::moveForward(float distance)
{
    translate(forward() * distance);
}

void Transform::moveBack(float distance)
{
    translate(back() * distance);
}

void Transform::moveRight(float distance)
{
    translate(right() * distance);
}

void Transform::moveLeft(float distance)
{
    translate(left() * distance);
}

void Transform::moveUp(float distance)
{
    translate(up() * distance);
}

void Transform::moveDown(float distance)
{
    translate(down() * distance);
}

void Transform::strafe(float rightDist, float upDist)
{
    translate(right() * rightDist + up() * upDist);
}

void Transform::strafe(float rightDist, float forwardDist, float upDist)
{
    translate(right() * rightDist + forward() * forwardDist + up() * upDist);
}

// ==================== Rotação ====================

void Transform::rotate(const Quat &rotation)
{
    setRotation(rotation * getRotation());
}

void Transform::rotate(const Vec3 &axis, float degrees)
{
    rotate(Quat::FromAxisAngleDeg(axis, degrees));
}

void Transform::rotateX(float degrees)
{
    rotate(Vec3(1, 0, 0), degrees);
}

void Transform::rotateY(float degrees)
{
    rotate(Vec3(0, 1, 0), degrees);
}

void Transform::rotateZ(float degrees)
{
    rotate(Vec3(0, 0, 1), degrees);
}

void Transform::rotateLocal(const Quat &rotation)
{
    setLocalRotation(localRotation * rotation);
}

void Transform::rotateLocal(const Vec3 &axis, float degrees)
{
    rotateLocal(Quat::FromAxisAngleDeg(axis, degrees));
}

void Transform::rotateLocalX(float degrees)
{
    rotateLocal(Vec3(1, 0, 0), degrees);
}

void Transform::rotateLocalY(float degrees)
{
    rotateLocal(Vec3(0, 1, 0), degrees);
}

void Transform::rotateLocalZ(float degrees)
{
    rotateLocal(Vec3(0, 0, 1), degrees);
}

void Transform::rotateFPS(float pitch, float yaw)
{
    // Rotação estilo FPS: yaw em world Y, pitch em local X
    Quat yawRot = Quat::RotationYDeg(yaw);
    Quat pitchRot = Quat::RotationXDeg(pitch);

    setRotation(yawRot * getRotation() * pitchRot);
}

// ==================== Look At ====================

void Transform::lookAt(const Vec3 &target)
{
    lookAt(target, Vec3(0, 1, 0));
}

void Transform::lookAt(const Vec3 &target, const Vec3 &up)
{
    Vec3 direction = (target - getPosition()).normalized();
    lookDirection(direction, up);
}

void Transform::lookAt(const Transform &target)
{
    lookAt(target.worldPosition);
}

void Transform::lookDirection(const Vec3 &direction)
{
    lookDirection(direction, Vec3(0, 1, 0));
}

void Transform::lookDirection(const Vec3 &direction, const Vec3 &up)
{
    if (direction.lengthSquared() < 1e-6f)
        return;

    Vec3 forward = direction.normalized();
    Vec3 right = Vec3::Cross(up, forward).normalized();
    Vec3 newUp = Vec3::Cross(forward, right);

    // Criar matriz de rotação a partir dos eixos
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

// ==================== Interpolação ====================

Transform Transform::Lerp(const Transform &a, const Transform &b, float t)
{
    Transform result;
    result.setPosition(Vec3::Lerp(a.worldPosition, b.worldPosition, t));
    result.setRotation(Quat::Nlerp(a.worldRotation, b.worldRotation, t));
    result.setScale(Vec3::Lerp(a.worldScale, b.worldScale, t));
    return result;
}

Transform Transform::Slerp(const Transform &a, const Transform &b, float t)
{
    Transform result;
    result.setPosition(Vec3::Lerp(a.worldPosition, b.worldPosition, t));
    result.setRotation(Quat::Slerp(a.worldRotation, b.worldRotation, t));
    result.setScale(Vec3::Lerp(a.worldScale, b.worldScale, t));
    return result;
}

// ==================== Utilidades ====================

Vec3 Transform::transformPoint(const Vec3 &localPoint)
{
    updateWorldTransform();
    Vec4 point(localPoint, 1.0f);
    return worldMatrix * localPoint;
}

Vec3 Transform::transformDirection(const Vec3 &localDirection)
{
    return getRotation() * localDirection;
}

Vec3 Transform::inverseTransformPoint(const Vec3 &worldPoint)
{
    updateWorldTransform();
    Mat4 invMatrix = worldMatrix.inverse();
    return invMatrix * worldPoint;
}

Vec3 Transform::inverseTransformDirection(const Vec3 &worldDirection)
{
    return getRotation().inverse() * worldDirection;
}

void Transform::reset()
{
    setLocalPosition(Vec3(0, 0, 0));
    setLocalRotation(Quat::Identity());
    setLocalScale(Vec3(1, 1, 1));
}

void Transform::print() const
{
    // std::cout << "Transform:" << std::endl;
    // std::cout << "  Local Position: " << localPosition << std::endl;
    // std::cout << "  Local Rotation: " << localRotation << std::endl;
    // std::cout << "  Local Scale: " << localScale << std::endl;
    // std::cout << "  World Position: " << worldPosition << std::endl;
    // std::cout << "  World Rotation: " << worldRotation << std::endl;
    // std::cout << "  Parent: " << (parent ? "yes" : "no") << std::endl;
    // std::cout << "  Children: " << children.size() << std::endl;
}
