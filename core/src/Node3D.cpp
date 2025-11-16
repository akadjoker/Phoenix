#include "pch.h"
#include "Node3D.hpp"
 #include "Mesh.hpp"

void Node3D::serialize(Serialize &serialize)
{
    Node::serialize(serialize);

    if (m_parent)
    {
        serialize.SetString("parent", m_parent->getName());
    }
    serialize.SetInt("flags", (int)m_flags);
    serialize.SetVec3("position", m_localPosition);
    serialize.SetVec3("rotation", getEulerAngles());
    serialize.SetVec3("scale", m_localScale);
}

void Node3D::deserialize(const Serialize &in)
{
    Node::deserialize(in);
   // m_flags = in.GetInt("flags", 0);
   // setFlags(m_flags);
    Vec3 pos = in.GetVec3("position", Vec3(0));
    setPosition(pos);
    Vec3 eur = in.GetVec3("rotation", Vec3(0));
    setEulerAngles(eur);
    Vec3 scale = in.GetVec3("scale", Vec3(1));
    setScale(scale);
    getWorldTransform();
}

Node3D::Node3D(const std::string &name) : Node(name), m_localPosition(0, 0, 0), m_localRotation(Quat::Identity()), m_localScale(1, 1, 1), m_transformDirty(true), m_worldTransformDirty(true), m_parent(nullptr),
                                          m_flags(0u)
{
    m_boundBox = BoundingBox(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
    setPickable(true);
 
}

 

Node3D::~Node3D()
{
   
    for (Node3D* child : m_children)
    {
        child->m_parent = nullptr;
    }
    
 
    if (m_parent)
    {
        m_parent->removeChild(this);
    }
}

 

void Node3D::setFlag(NodeFlag flag, bool enabled)
{
    u32 bit = static_cast<u32>(flag);
    if (enabled)
        m_flags |= bit;
    else
        m_flags &= ~bit;
}

void Node3D::clearFlag(NodeFlag flag)
{
    setFlag(flag, false);
}

bool Node3D::hasFlag(NodeFlag flag) const
{
    u32 bit = static_cast<u32>(flag);
    return (m_flags & bit) != 0;
}

void Node3D::setPickable(bool enabled)
{
    setFlag(NodeFlag::Pickable, enabled);
}

bool Node3D::isPickable() const
{
    return hasFlag(NodeFlag::Pickable);
}

void Node3D::setShowBoxes(bool enabled)
{
    setFlag(NodeFlag::ShowBox, enabled);
}

bool Node3D::isShowBoxes() const
{
    return hasFlag(NodeFlag::ShowBox);
}

bool Node3D::pick(const Ray &ray) const
{
    if (!isPickable())
        return false;
    
    float tMin, tMax;

    BoundingBox box = getTransformedBoundingBox();
    return ray.intersectAABB(box, tMin, tMax);
}


void Node3D::setFlags(u32 flags)
{
    m_flags = flags;
}

u32 Node3D::getFlags() const
{
    return m_flags;
}


void Node3D::updateLocalTransform() const
{
    Mat4 translation = Mat4::Translation(m_localPosition);
    Mat4 rotation = m_localRotation.toMat4();
    Mat4 scale = Mat4::Scale(m_localScale);
    
    m_localTransform = translation * rotation * scale;
    m_transformDirty = false;
}

void Node3D::updateWorldTransform() const
{
    if (m_transformDirty)
        updateLocalTransform();
    
    if (m_parent)
        m_worldTransform = m_parent->getWorldTransform() * m_localTransform;
    else
        m_worldTransform = m_localTransform;
    
    m_worldTransformDirty = false;
}

void Node3D::markDirty()
{
    m_transformDirty = true;
    markWorldDirty();
}

void Node3D::markWorldDirty()
{
    m_worldTransformDirty = true;
    
    // Propagar para todos os filhos
    for (Node3D* child : m_children)
    {
        child->markWorldDirty();
    }
}

// Transform getters/setters
void Node3D::setPosition(const Vec3& pos, TransformSpace space)
{
    switch (space)
    {
        case TransformSpace::Local:
            m_localPosition = pos;
            break;
            
        case TransformSpace::Parent:
            m_localPosition = pos;
            break;
            
        case TransformSpace::World:
            if (m_parent)
            {
                Mat4 parentInverse = m_parent->getWorldTransform().inverse();
                m_localPosition = parentInverse.TransformPoint(pos);
            }
            else
            {
                m_localPosition = pos;
            }
            break;
    }
    
    markDirty();
}

void Node3D::setPosition(float x, float y, float z, TransformSpace space) 
{

    switch (space)
    {
        case TransformSpace::Local:
        {
            m_localPosition.x=x;
            m_localPosition.y=y;
            m_localPosition.z=z;
            break;
        }   
        case TransformSpace::Parent:
        {
            m_localPosition.x=x;
            m_localPosition.y=y;
            m_localPosition.z=z;
            break;
        }
            
        case TransformSpace::World:
        {
            if (m_parent)
            {
                Vec3 pos(x,y,z);
                Mat4 parentInverse = m_parent->getWorldTransform().inverse();
                m_localPosition = parentInverse.TransformPoint(pos);
            }
            else
            {
            m_localPosition.x=x;
            m_localPosition.y=y;
            m_localPosition.z=x;

            }
            break;
        }
    }
    
    markDirty();
}

Vec3 Node3D::getPosition(TransformSpace space) const
{
    switch (space)
    {
        case TransformSpace::Local:
        case TransformSpace::Parent:
            return m_localPosition;
            
        case TransformSpace::World:
        {
            const Mat4& world = getWorldTransform();
            return Vec3(world[12], world[13], world[14]);
        }
    }
    
    return m_localPosition;
}

void Node3D::setRotation(const Quat& rot, TransformSpace space)
{
    switch (space)
    {
        case TransformSpace::Local:
        case TransformSpace::Parent:
            m_localRotation = rot;
            break;
            
        case TransformSpace::World:
            if (m_parent)
            {
                Quat parentWorldRot = m_parent->getRotation(TransformSpace::World);
                m_localRotation = parentWorldRot.inverse() * rot;
            }
            else
            {
                m_localRotation = rot;
            }
            break;
    }
    
    markDirty();
}

Quat Node3D::getRotation(TransformSpace space) const
{
    switch (space)
    {
        case TransformSpace::Local:
        case TransformSpace::Parent:
            return m_localRotation;
            
        case TransformSpace::World:
            if (m_parent)
            {
                Quat parentWorldRot = m_parent->getRotation(TransformSpace::World);
                return parentWorldRot * m_localRotation;
            }
            return m_localRotation;
    }
    
    return m_localRotation;
}

void Node3D::setScale(const Vec3& scale)
{
    m_localScale = scale;
    markDirty();
}

Vec3 Node3D::getScale(TransformSpace space) const
{
    switch (space)
    {
        case TransformSpace::Local:
        case TransformSpace::Parent:
            return m_localScale;
            
        case TransformSpace::World:
            // Extrair scale da world matrix é complexo com rotação
            // Simplificado: assume uniform scale ou retorna local
            if (m_parent)
            {
                Vec3 parentScale = m_parent->getScale(TransformSpace::World);
                return Vec3(m_localScale.x * parentScale.x,
                           m_localScale.y * parentScale.y,
                           m_localScale.z * parentScale.z);
            }
            return m_localScale;
    }
    
    return m_localScale;
}

// Euler angles
void Node3D::setEulerAngles(const Vec3& euler)
{
    m_localRotation = Quat::FromEulerAngles(euler);
    markDirty();
}

void Node3D::setEulerAnglesDeg(const Vec3& eulerDeg)
{
    setEulerAngles(eulerDeg * DEG2RAD);
}

Vec3 Node3D::getEulerAngles() const
{
    return m_localRotation.toEulerAngles();
}

Vec3 Node3D::getEulerAnglesDeg() const
{
    return getEulerAngles() * RAD2DEG;
}

void Node3D::setPitch(float pitch)
{
    Vec3 euler = getEulerAngles();
    euler.x = pitch;
    setEulerAngles(euler);
}

void Node3D::setYaw(float yaw)
{
    Vec3 euler = getEulerAngles();
    euler.y = yaw;
    setEulerAngles(euler);
}

void Node3D::setRoll(float roll)
{
    Vec3 euler = getEulerAngles();
    euler.z = roll;
    setEulerAngles(euler);
}

void Node3D::setPitchDeg(float pitchDeg)
{
    setPitch(pitchDeg * DEG2RAD);
}

void Node3D::setYawDeg(float yawDeg)
{
    setYaw(yawDeg * DEG2RAD);
}

void Node3D::setRollDeg(float rollDeg)
{
    setRoll(rollDeg * DEG2RAD);
}

float Node3D::getPitch() const
{
    return getEulerAngles().x;
}

float Node3D::getYaw() const
{
    return getEulerAngles().y;
}

float Node3D::getRoll() const
{
    return getEulerAngles().z;
}

float Node3D::getPitchDeg() const
{
    return getPitch() * RAD2DEG;
}

float Node3D::getYawDeg() const
{
    return getYaw() * RAD2DEG;
}

float Node3D::getRollDeg() const
{
    return getRoll() * RAD2DEG;
}

// Transformações incrementais
void Node3D::translate(const Vec3& offset, TransformSpace space)
{
    switch (space)
    {
        case TransformSpace::Local:
            m_localPosition += m_localRotation * offset;
            break;
            
        case TransformSpace::Parent:
            m_localPosition += offset;
            break;
            
        case TransformSpace::World:
            if (m_parent)
            {
                Quat parentWorldRot = m_parent->getRotation(TransformSpace::World);
                m_localPosition += parentWorldRot.inverse() * offset;
            }
            else
            {
                m_localPosition += offset;
            }
            break;
    }
    
    markDirty();
}

void Node3D::rotate(const Quat& rot, TransformSpace space)
{
    switch (space)
    {
        case TransformSpace::Local:
            m_localRotation = m_localRotation * rot;
            break;
            
        case TransformSpace::Parent:
            m_localRotation = rot * m_localRotation;
            break;
            
        case TransformSpace::World:
            if (m_parent)
            {
                Quat parentWorldRot = m_parent->getRotation(TransformSpace::World);
                m_localRotation = parentWorldRot.inverse() * rot * parentWorldRot * m_localRotation;
            }
            else
            {
                m_localRotation = rot * m_localRotation;
            }
            break;
    }
    
    m_localRotation.normalize();
    markDirty();
}

void Node3D::rotate(const Vec3& axis, float angleRad, TransformSpace space)
{
    Quat rot = Quat::FromAxisAngle(axis, angleRad);
    rotate(rot, space);
}

void Node3D::rotateDeg(const Vec3& axis, float angleDeg, TransformSpace space)
{
    rotate(axis, angleDeg * DEG2RAD, space);
}

void Node3D::scale(const Vec3& scale)
{
    m_localScale = Vec3(m_localScale.x * scale.x,
                        m_localScale.y * scale.y,
                        m_localScale.z * scale.z);
    markDirty();
}

// Matrizes
const Mat4& Node3D::getLocalTransform() const
{
    if (m_transformDirty)
        updateLocalTransform();
    
    return m_localTransform;
}

const Mat4& Node3D::getWorldTransform() const
{
   if (m_worldTransformDirty)
        updateWorldTransform();
    
    return m_worldTransform;
}

// Hierarquia
void Node3D::setParent(Node3D* parent)
{
    // Check circular dependency
    Node3D* p = parent;
    while (p)
    {
        if (p == this)
            return; // Circular dependency detected
        p = p->m_parent;
    }
    
    // Remove from old parent
    if (m_parent)
        m_parent->removeChild(this);
    
    m_parent = parent;
    
    if (m_parent)
        m_parent->addChild(this);
    
    markWorldDirty();
}

void Node3D::addChild(Node3D* child)
{
    if (!child || child == this)
        return;
    
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it == m_children.end())
    {
        m_children.push_back(child);
        child->m_parent = this;
        child->markWorldDirty();
    }
}

void Node3D::removeChild(Node3D* child)
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end())
    {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void Node3D::removeFromParent()
{
    if (m_parent)
        m_parent->removeChild(this);
}

// Direções
Vec3 Node3D::getForward(TransformSpace space) const
{
    Vec3 forward(0, 0, -1);
    
    switch (space)
    {
        case TransformSpace::Local:
            return forward;
            
        case TransformSpace::Parent:
            return m_localRotation * forward;
            
        case TransformSpace::World:
        {
            Quat worldRot = getRotation(TransformSpace::World);
            return worldRot * forward;
        }
    }
    
    return forward;
}

Vec3 Node3D::getRight(TransformSpace space) const
{
    Vec3 right(1, 0, 0);
    
    switch (space)
    {
        case TransformSpace::Local:
            return right;
            
        case TransformSpace::Parent:
            return m_localRotation * right;
            
        case TransformSpace::World:
        {
            Quat worldRot = getRotation(TransformSpace::World);
            return worldRot * right;
        }
    }
    
    return right;
}

Vec3 Node3D::getUp(TransformSpace space) const
{
    Vec3 up(0, 1, 0);
    
    switch (space)
    {
        case TransformSpace::Local:
            return up;
            
        case TransformSpace::Parent:
            return m_localRotation * up;
            
        case TransformSpace::World:
        {
            Quat worldRot = getRotation(TransformSpace::World);
            return worldRot * up;
        }
    }
    
    return up;
}

// LookAt
void Node3D::lookAt(const Vec3& target, TransformSpace targetSpace, const Vec3& up)
{
    Vec3 worldTarget = target;
    
    // Converter target para world space se necessário
    if (targetSpace == TransformSpace::Local && m_parent)
    {
        worldTarget = m_parent->getWorldTransform().TransformPoint(target);
    }
    else if (targetSpace == TransformSpace::Parent && m_parent)
    {
        worldTarget = m_parent->getWorldTransform().TransformPoint(target);
    }
    
    Vec3 worldPos = getPosition(TransformSpace::World);
    Vec3 forward = (worldTarget - worldPos).normalized();
    
    // Evitar lookAt quando target == position
    if (forward.lengthSquared() < 0.0001f)
        return;
    
    Vec3 right = Vec3::Cross(up, forward).normalized();
    Vec3 newUp = Vec3::Cross(forward, right);
    
    // Criar matriz de rotação
    Mat4 lookAtMat = Mat4::Identity();
    lookAtMat[0] = right.x;    lookAtMat[4] = right.y;    lookAtMat[8] = right.z;
    lookAtMat[1] = newUp.x;    lookAtMat[5] = newUp.y;    lookAtMat[9] = newUp.z;
    lookAtMat[2] = -forward.x; lookAtMat[6] = -forward.y; lookAtMat[10] = -forward.z;
    
    Quat worldRot = Quat::FromMat4(lookAtMat);
    setRotation(worldRot, TransformSpace::World);
}

const BoundingBox Node3D::getTransformedBoundingBox() const
{
    return BoundingBox::Transform(getBoundingBox(),getWorldTransform());
}

void Node3D::rotateX(float angleRad, TransformSpace space)
{
    rotate(Vec3(1.0f, 0.0f, 0.0f), angleRad, space);
}

void Node3D::rotateY(float angleRad, TransformSpace space)
{
    rotate(Vec3(0.0f, 1.0f, 0.0f), angleRad, space);
}

void Node3D::rotateZ(float angleRad, TransformSpace space)
{
    rotate(Vec3(0.0f, 0.0f, 1.0f), angleRad, space);
}

void Node3D::rotateXDeg(float angleDeg, TransformSpace space)
{
    rotateX(angleDeg * DEG2RAD, space);
}

void Node3D::rotateYDeg(float angleDeg, TransformSpace space)
{
    rotateY(angleDeg * DEG2RAD, space);
}

void Node3D::rotateZDeg(float angleDeg, TransformSpace space)
{
    rotateZ(angleDeg * DEG2RAD, space);
}

