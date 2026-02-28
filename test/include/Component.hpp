#pragma once
#include "Config.hpp"



class GameObject;

class Component
{
protected:
    friend class GameObject;
    
    GameObject* m_owner;
    bool m_enabled;
    bool m_started;

public:
    Component();
    virtual ~Component() = default;

    // Non-copyable, movable
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    // ==================== Getters/Setters ====================
    
    GameObject* getOwner() const { return m_owner; }
    void setOwner(GameObject* owner) { m_owner = owner; }
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    
    bool isStarted() const { return m_started; }

    // ==================== Lifecycle ====================
    
    /**
     * @brief Called when component is first attached (before start)
     */
    virtual void awake() {}
    
    /**
     * @brief Called before first update (after all awakes)
     */
    virtual void start() {}
    
    /**
     * @brief Called every frame
     */
    virtual void update(float deltaTime) {}
    
    /**
     * @brief Called after all updates
     */
    virtual void lateUpdate(float deltaTime) {}
    
    /**
     * @brief Called every frame for rendering
     */
    virtual void render() {}
    
    /**
     * @brief Called when component is enabled
     */
    virtual void onEnable() {}
    
    /**
     * @brief Called when component is disabled
     */
    virtual void onDisable() {}
    
    /**
     * @brief Called before component is destroyed
     */
    virtual void onDestroy() {}

    // ==================== Type Information ====================
    
    /**
     * @brief Returns component type name
     */
    virtual const char* getTypeName() const = 0;

protected:
    /**
     * @brief Called when component is attached to GameObject
     */
    virtual void attach() {}
    
    /**
     * @brief Called when component is detached from GameObject
     */
    virtual void detach() {}
};