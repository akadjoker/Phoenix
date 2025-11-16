#include "pch.h"
#include "Utils.hpp"
#include "Object.hpp"
#include "Stream.hpp"



u32 Object::s_nextId = 1;

Property::Property(const std::string &val) : type(STRING), str(val) {}
Property::Property(Vec2 val) : type(VEC2), v2(val) {}
Property::Property(Vec3 val) : type(VEC3), v3(val) {}
Property::Property(Vec4 val) : type(VEC4), v4(val) {}
 

void Object::serialize(Serialize &serialize)
{
    serialize.SetString("name", m_name);
    serialize.SetBool("active", m_active);
    serialize.SetInt("type", (int)getType());
}

void Object::deserialize(const Serialize &in)
{
    m_name = in.GetString("name", "Object");
    m_active =in.GetBool("active", true);
}

Object::Object(const std::string &name)
    : m_id(generateId()), m_name(name), m_active(true)
{
}

u32 Object::generateId()
{
    return s_nextId++;
}

void Object::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (m_active)
        onActivate();
    else
        onDeactivate();
}

bool Object::isActive() const
{
    return m_active;
}

// Setters
void Serialize::SetInt(const std::string &key, int value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetFloat(const std::string &key, float value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetBool(const std::string &key, bool value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetString(const std::string &key, const std::string &value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetVec2(const std::string &key, const Vec2 &value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetVec3(const std::string &key, const Vec3 &value)
{
    m_properties[key] = Property(value);
}

void Serialize::SetVec4(const std::string &key, const Vec4 &value)
{
    m_properties[key] = Property(value);
}

 
// Getters
int Serialize::GetInt(const std::string &key, int defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::INT)
        return it->second.i;
    return defaultValue;
}

float Serialize::GetFloat(const std::string &key, float defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::FLOAT)
        return it->second.f;
    return defaultValue;
}

bool Serialize::GetBool(const std::string &key, bool defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::BOOL)
        return it->second.b;
    return defaultValue;
}

std::string Serialize::GetString(const std::string &key, const std::string &defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::STRING)
        return it->second.str;
    return defaultValue;
}

Vec2 Serialize::GetVec2(const std::string &key, const Vec2 &defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::VEC2)
        return it->second.v2;
    return defaultValue;
}

Vec3 Serialize::GetVec3(const std::string &key, const Vec3 &defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::VEC3)
        return it->second.v3;
    return defaultValue;
}

Vec4 Serialize::GetVec4(const std::string &key, const Vec4 &defaultValue) const
{
    auto it = m_properties.find(key);
    if (it != m_properties.end() && it->second.type == Property::VEC4)
        return it->second.v4;
    return defaultValue;
}

 

// Utils
bool Serialize::HasProperty(const std::string &key) const
{
    return m_properties.find(key) != m_properties.end();
}

void Serialize::RemoveProperty(const std::string &key)
{
    m_properties.erase(key);
}

void Serialize::ClearProperties()
{
    m_properties.clear();
}
