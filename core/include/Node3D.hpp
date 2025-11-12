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

enum class TransformSpace
{
    Local,
    Parent,
    World
};

class Node3D : public Node
{
protected:
    Vec3 m_localPosition;
    Quat m_localRotation;
    Vec3 m_localScale;

    mutable Mat4 m_localTransform;
    mutable Mat4 m_worldTransform;
    mutable bool m_transformDirty;
    mutable bool m_worldTransformDirty;

    Node3D *m_parent;
    std::vector<Node3D *> m_children;

    void updateLocalTransform() const;
    void updateWorldTransform() const;
    void markDirty();
    void markWorldDirty();

public:
    Node3D(const std::string &name = "Node3D");
    virtual ~Node3D();

    // Transform
    virtual void setPosition(const Vec3 &pos, TransformSpace space = TransformSpace::Local);
    virtual void setPosition(float x, float y, float z, TransformSpace space = TransformSpace::Local);
    virtual void setRotation(const Quat &rot, TransformSpace space = TransformSpace::Local);
    void setScale(const Vec3 &scale);
    void setEulerAngles(const Vec3 &euler);
    void setEulerAnglesDeg(const Vec3 &eulerDeg);

    Vec3 getPosition(TransformSpace space = TransformSpace::Local) const;
    Quat getRotation(TransformSpace space = TransformSpace::Local) const;
    Vec3 getScale(TransformSpace space = TransformSpace::Local) const;
    Vec3 getEulerAngles() const;
    Vec3 getEulerAnglesDeg() const;

    // Euler individual
    void setPitch(float pitch);
    void setYaw(float yaw);
    void setRoll(float roll);
    void setPitchDeg(float pitchDeg);
    void setYawDeg(float yawDeg);
    void setRollDeg(float rollDeg);

    float getPitch() const;
    float getYaw() const;
    float getRoll() const;
    float getPitchDeg() const;
    float getYawDeg() const;
    float getRollDeg() const;

    // Transformações incrementais
    virtual void translate(const Vec3 &offset, TransformSpace space = TransformSpace::Local);
    virtual void rotate(const Quat &rot, TransformSpace space = TransformSpace::Local);
    virtual void rotate(const Vec3 &axis, float angleRad, TransformSpace space = TransformSpace::Local);
    virtual void rotateDeg(const Vec3 &axis, float angleDeg, TransformSpace space = TransformSpace::Local);
    void scale(const Vec3 &scale);

    // Matrizes
    const Mat4 &getLocalTransform() const;
    const Mat4 &getWorldTransform() const;

    virtual void update(float deltaTime) override {};
    virtual void render() override {}

    // Hierarquia
    void setParent(Node3D *parent);
    Node3D *getParent() const { return m_parent; }
    const std::vector<Node3D *> &getChildren() const { return m_children; }
    void addChild(Node3D *child);
    void removeChild(Node3D *child);
    void removeFromParent();

    // Direções
    Vec3 getForward(TransformSpace space = TransformSpace::Local) const;
    Vec3 getRight(TransformSpace space = TransformSpace::Local) const;
    Vec3 getUp(TransformSpace space = TransformSpace::Local) const;

    // Utilitários
    virtual void lookAt(const Vec3 &target, TransformSpace targetSpace = TransformSpace::World, const Vec3 &up = Vec3(0, 1, 0));
};