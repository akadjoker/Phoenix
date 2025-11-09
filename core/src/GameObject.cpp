#include "pch.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Math.hpp"
#include "Node.hpp"
#include "Node3D.hpp"
#include "Component.hpp"
#include "GameObject.hpp"
 
#include "GameObject.hpp"
#include <algorithm>

GameObject::GameObject(const std::string& name)
    : Node3D(name),
      m_started(false)
{
}

GameObject::~GameObject()
{
    removeAllComponents();
}

// ==================== Component Management ====================

void GameObject::removeAllComponents()
{
    // Call onDestroy on all components
    for (Component* component : m_components)
    {
        component->onDestroy();
        component->detach();
        delete component;
    }
    
    m_components.clear();
    m_componentsByType.clear();
}

// ==================== Lifecycle ====================

void GameObject::awake()
{
    // Awake already called when components are added
    // This is for GameObject-specific awake logic
}

void GameObject::start()
{
    if (m_started)
        return;
    
    m_started = true;
    
    // Start all components
    for (Component* component : m_components)
    {
        if (component->isEnabled() && !component->isStarted())
        {
            component->start();
            component->m_started = true;
        }
    }
}

void GameObject::update(float deltaTime)
{
    updateWorldTransform();
    
    
    Node3D::update(deltaTime);

   // LogInfo("[GameObject] update %s", getName().c_str());
    
    // Start if not started yet
    if (!m_started)
    {
        start();
    }
    
    // Update all enabled components
    for (Component* component : m_components)
    {
        if (component->isEnabled())
        {
            component->update(deltaTime);
        }
    }
}

void GameObject::lateUpdate(float deltaTime)
{
    // Late update for all enabled components
    for (Component* component : m_components)
    {
        if (component->isEnabled())
        {
            component->lateUpdate(deltaTime);
        }
    }
}

void GameObject::render()
{
     
    Node3D::render();

    //LogInfo("[GameObject] render %s", getName().c_str());
    
    // Render all enabled components
    for (Component* component : m_components)
    {
        if (component->isEnabled())
        {
            component->render();
        }
    }
}

// ==================== Private Methods ====================

void GameObject::destroyComponent(Component* component)
{
    if (!component)
        return;
    
    // Remove from main list
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
    }
    
    // Remove from type map
    auto typeIt = m_componentsByType.find(std::type_index(typeid(*component)));
    if (typeIt != m_componentsByType.end())
    {
        auto& typeComponents = typeIt->second;
        auto compIt = std::find(typeComponents.begin(), typeComponents.end(), component);
        if (compIt != typeComponents.end())
        {
            typeComponents.erase(compIt);
        }
        
        // Remove empty type entry
        if (typeComponents.empty())
        {
            m_componentsByType.erase(typeIt);
        }
    }
    
    // Cleanup component
    component->onDestroy();
    component->detach();
    delete component;
}