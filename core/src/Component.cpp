#include "pch.h"
#include "Component.hpp"

Component::Component()
    : m_owner(nullptr),
      m_enabled(true),
      m_started(false)
{
}

void Component::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    
    m_enabled = enabled;
    
    if (m_enabled)
        onEnable();
    else
        onDisable();
}
