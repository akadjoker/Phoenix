#include "pch.h"
#include "Object.hpp"
 

u32 Object::s_nextId = 1;  


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

std::string Object::toString() const
{
    std::ostringstream oss;
    oss << m_name << " (ID: " << m_id << ")";
    return oss.str();
}

bool Object::isActive() const noexcept
{
    return m_active;
}