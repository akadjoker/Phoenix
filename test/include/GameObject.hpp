#pragma once
#include "Config.hpp"
#include "Node3D.hpp"
#include "Component.hpp"

#include <vector>
#include <map>
#include <typeindex>
#include <algorithm>

/**
 * @brief GameObject with component system (Unity-style)
 * 
 * GameObject extends Node3D and adds a component system for composition.
 * Components can be added/removed dynamically to add functionality.
 */
class GameObject : public Node3D
{
private:
    std::map<std::type_index, std::vector<Component*>> m_componentsByType;
    std::vector<Component*> m_components;
    bool m_started;

    
    public:
    explicit GameObject(const std::string& name = "GameObject");
    ~GameObject() override;
    
    
    virtual  void serialize(Serialize& serialize) override; ;
    virtual void deserialize(const Serialize& in) override;

    // Non-copyable but movable
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;

    virtual ObjectType getType() override { return ObjectType::GameObject; }

    // ==================== Component Management ====================
    
    /**
     * @brief Gets first component of type T
     * @return Component pointer or nullptr if not found
     */
    template<typename T>
    T* getComponent();
    
    /**
     * @brief Checks if GameObject has component of type T
     */
    template<typename T>
    bool hasComponent() const;
    
    /**
     * @brief Counts components of type T
     */
    template<typename T>
    size_t countComponents() const;
    
    /**
     * @brief Gets all components of type T
     */
    template<typename T>
    std::vector<T*> getComponents();
    
    /**
     * @brief Adds new component (created in-place)
     */
    template<typename T, typename... Args>
    T* addComponent(Args&&... args);
    
    /**
     * @brief Adds existing component
     */
    template<typename T>
    T* addComponent(T* component);
    
    /**
     * @brief Removes first component of type T
     */
    template<typename T>
    bool removeComponent();
    
    /**
     * @brief Removes specific component instance
     */
    template<typename T>
    bool removeComponent(T* component);
    
    /**
     * @brief Removes all components
     */
    void removeAllComponents();
    
    /**
     * @brief Returns all components
     */
    const std::vector<Component*>& getAllComponents() const { return m_components; }

    // ==================== Lifecycle ====================
    
    /**
     * @brief Called when GameObject is created (before start)
     */
    void awake();
    
    /**
     * @brief Called before first update
     */
    void start();
    
    /**
     * @brief Updates GameObject and all components
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Late update (after all updates)
     */
    void lateUpdate(float deltaTime);
    
    /**
     * @brief Renders GameObject and all components
     */
    void render() override;

private:
    void destroyComponent(Component* component);

protected:
   
};

// ==================== Template Implementations ====================

template<typename T>
inline T* GameObject::getComponent()
{
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
    
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    if (it == m_componentsByType.end() || it->second.empty())
    {
        return nullptr;
    }
    return static_cast<T*>(it->second[0]);
}

template<typename T>
inline bool GameObject::hasComponent() const
{
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    return it != m_componentsByType.end() && !it->second.empty();
}

template<typename T>
inline size_t GameObject::countComponents() const
{
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    return (it != m_componentsByType.end()) ? it->second.size() : 0;
}

template<typename T>
inline std::vector<T*> GameObject::getComponents()
{
    std::vector<T*> result;
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    if (it == m_componentsByType.end())
    {
        return result;
    }
    
    result.reserve(it->second.size());
    for (Component* component : it->second)
    {
        result.push_back(static_cast<T*>(component));
    }
    return result;
}

template<typename T, typename... Args>
inline T* GameObject::addComponent(Args&&... args)
{
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
    
    T* component = new T(std::forward<Args>(args)...);
    
    component->m_owner = this;
    m_components.push_back(component);
    m_componentsByType[std::type_index(typeid(T))].push_back(component);
    
    component->attach();
    component->awake();
    
    if (m_started)
    {
        component->start();
        component->m_started = true;
    }
    
    return component;
}

template<typename T>
inline T* GameObject::addComponent(T* component)
{
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
    
    if (!component)
        return nullptr;
    
    // Avoid duplicates
    if (std::find(m_components.begin(), m_components.end(), component) != m_components.end())
        return component;
    
    component->m_owner = this;
    m_components.push_back(component);
    m_componentsByType[std::type_index(typeid(T))].push_back(component);
    
    component->attach();
    component->awake();
    
    if (m_started)
    {
        component->start();
        component->m_started = true;
    }
    
    return component;
}

template<typename T>
inline bool GameObject::removeComponent()
{
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    if (it == m_componentsByType.end() || it->second.empty())
    {
        return false;
    }
    
    Component* component = it->second[0];
    destroyComponent(component);
    return true;
}

template<typename T>
inline bool GameObject::removeComponent(T* component)
{
    if (!component)
        return false;
    
    auto it = m_componentsByType.find(std::type_index(typeid(T)));
    if (it == m_componentsByType.end())
        return false;
    
    auto& components = it->second;
    for (auto compIt = components.begin(); compIt != components.end(); ++compIt)
    {
        if (*compIt == component)
        {
            destroyComponent(component);
            return true;
        }
    }
    return false;
}