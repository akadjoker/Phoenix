#pragma once
#include "Config.hpp"
#include "Node.hpp"
#include "Math.hpp"
#include "Ray.hpp"

/**
 * @brief 3D spatial node with transformation capabilities
 *
 * Node3D extends Node with 3D position, rotation, and scale.
 * All 3D objects (cameras, meshes, lights) inherit from Node3D.
 */

class MeshRenderer;
class TerrainRenderer;
class AnimationLayer;
class Animator;
class Node3D;
class Joint3D;

enum class TransformSpace
{
    Local,
    Parent,
    World
};

enum class NodeFlag : u32
{
    None = 0,
    Pickable = 1u << 0,

    Visible = 1u << 1,
    Static = 1u << 2,
    CastShadows = 1u << 3,
    ReceiveShadows = 1u << 4,
    ShowBox = 1u << 5,

};





class Node3D : public Node
{
protected:
    Vec3 m_localPosition;
    Quat m_localRotation;
    Vec3 m_localScale;

    bool m_overrideLocalTransform;
    mutable Mat4 m_localTransform;
    mutable Mat4 m_worldTransform;
    mutable bool m_transformDirty;
    mutable bool m_worldTransformDirty;

    Node3D *m_parent;
    u32 m_flags;
    std::vector<Node3D *> m_children;
    std::vector<Joint3D *> m_joints;
    std::unordered_map<std::string, Joint3D *> m_jointsMap;

 

    virtual void updateLocalTransform() const;
    virtual void updateWorldTransform() const;
    void markDirty();
    void markWorldDirty();

    void setFlag(NodeFlag flag, bool enabled = true);
    void clearFlag(NodeFlag flag);
    bool hasFlag(NodeFlag flag) const;

    void setFlags(u32 flags);
    u32 getFlags() const;

    friend class MeshRenderer;
    friend class TerrainRenderer;
    friend class Animator;
    friend class AnimationLayer;
public:
    Node3D(const std::string &name = "Node3D");
    virtual ~Node3D();


    Joint3D *addJoint(const std::string &name);

    Joint3D *getJoint(const std::string &name) const;
    Joint3D *getJoint(u32 index) const;
    Joint3D *findJoint(const std::string &name) const; 
    u32 getJointCount() const { return (u32)m_joints.size(); }

    virtual void serialize(Serialize &serialize) override;
    virtual void deserialize(const Serialize &in) override;

    virtual ObjectType getType() override { return ObjectType::Node3D; }

    // Transform
    virtual void setPosition(const Vec3 &pos, TransformSpace space = TransformSpace::Local);
    virtual void setPosition(float x, float y, float z, TransformSpace space = TransformSpace::Local);
    virtual void setRotation(const Quat &rot, TransformSpace space = TransformSpace::Local);
    void setScale(const Vec3 &scale);
    void setScale(float x, float y, float z);
    void setScale(float value);
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

    void rotateX(float angleRad, TransformSpace space = TransformSpace::Local);
    void rotateY(float angleRad, TransformSpace space = TransformSpace::Local);
    void rotateZ(float angleRad, TransformSpace space = TransformSpace::Local);

    void rotateXDeg(float angleDeg, TransformSpace space = TransformSpace::Local);
    void rotateYDeg(float angleDeg, TransformSpace space = TransformSpace::Local);
    void rotateZDeg(float angleDeg, TransformSpace space = TransformSpace::Local);



    // Matrizes

    void setLocalTransform(const Mat4 &mat);
    virtual const Mat4 &getLocalTransform() const;
    virtual const Mat4 &getWorldTransform() const;


    virtual void update(float deltaTime) override {};
    virtual void render() override {}

    void setParent(Node3D *parent);
    Node3D *getParent() const { return m_parent; }
    const std::vector<Node3D *> &getChildren() const { return m_children; }
    void addChild(Node3D *child);
    void removeChild(Node3D *child);
    void removeFromParent();

    Vec3 getForward(TransformSpace space = TransformSpace::Local) const;
    Vec3 getRight(TransformSpace space = TransformSpace::Local) const;
    Vec3 getUp(TransformSpace space = TransformSpace::Local) const;

    virtual void lookAt(const Vec3 &target, TransformSpace targetSpace = TransformSpace::World, const Vec3 &up = Vec3(0, 1, 0));


    const BoundingBox getTransformedBoundingBox() const;

    void setPickable(bool enabled);
    bool isPickable() const;
    void setShowBoxes(bool enabled);
    bool isShowBoxes() const;

    bool pick(const Ray &ray) const;
};



class Joint3D : public Node3D
{
    bool hasAnimation;
    int parentIndex;
    Joint3D *parent;//model space 
    bool m_manualUpdate;

    mutable Mat4 m_modelTransform;
    mutable Mat4 m_globalTransform;
    
    mutable Mat4 m_localPose;// Local pose
    mutable Mat4 m_inverseBindPose; // Inverse bind pose

    friend class Animator;
    friend class MeshRenderer;
    friend class AnimationLayer;

    void SetAnimationFrame(const Vec3 &position, const Quat &rotation);
    void Reset();

    public:
        Joint3D(const std::string &name = "Joint3D");
        void SetControllable(bool value);
        bool IsControllable();

        void SetParent(Joint3D *parent);

        const Mat4& GetLocalPose() const ;
        const Mat4& GetBindPose() const ;

        const Mat4& GetGlobalTransform() const ;
        const Mat4& GetModelTransform() const ;


        virtual ObjectType getType() override { return ObjectType::Joint3D; }
};
