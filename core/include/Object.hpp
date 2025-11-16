#pragma once
#include "Config.hpp"
#include "Math.hpp"
 

class TextFile;
class string;
class Object;
 

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


struct Property
{
    enum Type { INT, FLOAT, STRING, VEC2, VEC3, VEC4, BOOL };
    
    Type type;
    union {
        int i;
        float f;
        bool b;
        Vec2 v2;
        Vec3 v3;
        Vec4 v4;
    
    };
    std::string str;  
    
    Property() : type(INT), i(0) {}
    Property(int val) : type(INT), i(val) {}
    Property(float val) : type(FLOAT), f(val) {}
    Property(bool val) : type(BOOL), b(val) {}
    Property(const std::string& val) ;
    Property(Vec2 val);  
    Property(Vec3 val);  
    Property(Vec4 val);
 
};

class Serialize
{
 

protected:
 
 
    std::unordered_map<std::string, Property> m_properties;
public:
 
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetBool(const std::string& key, bool value);
    void SetString(const std::string& key, const std::string& value);
    void SetVec2(const std::string& key, const Vec2& value);
    void SetVec3(const std::string& key, const Vec3& value);
    void SetVec4(const std::string& key, const Vec4& value);
 
    
    // Property Getters
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    Vec2 GetVec2(const std::string& key, const Vec2& defaultValue = Vec2(0)) const;
    Vec3 GetVec3(const std::string& key, const Vec3& defaultValue = Vec3(0)) const;
    Vec4 GetVec4(const std::string& key, const Vec4& defaultValue = Vec4(0)) const;
 
    
    // Property Utils
    bool HasProperty(const std::string& key) const;
    void RemoveProperty(const std::string& key);
    void ClearProperties();
    size_t GetPropertyCount() const { return m_properties.size(); }
    const std::unordered_map<std::string, Property>& GetAllProperties() const    {        return m_properties;    }
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
  
    virtual  void serialize(Serialize& serialize) ;
    virtual void deserialize(const Serialize& in);
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