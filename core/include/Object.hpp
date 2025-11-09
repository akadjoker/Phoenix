#pragma once
#include "Config.hpp"

class string;

enum class ObjectType : u8
{
    Unknown = 0,
    Object,
    Node,
    Node3D,
    GameObject,
    Mesh,
    Light,
    Camera,
    CameraFPS,
    CameraFree,
    CameraMaya,
    CameraOrbit,
    CameraThirdPerson,
    CameraThirdPersonSpring,
};

class Object
{
public:
    static constexpr u32 INVALID_ID = 0;

protected:
    static u32 s_nextId;
    u32 m_id;
    std::string m_name;
    bool m_active;

public:
    explicit Object(const std::string &name = "Object");
    virtual ~Object() = default;

    Object(const Object &) = delete;
    Object &operator=(const Object &) = delete;
    Object(Object &&) = default;
    Object &operator=(Object &&) = default;

    u32 getId() const { return m_id; }
    const std::string &getName() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    bool isActive() const  ;
    void setActive(bool active);

  
    virtual ObjectType getType() { return ObjectType::Object; }

protected:
    virtual void onActivate() {}
    virtual void onDeactivate() {}

protected:
    static u32 generateId();
};