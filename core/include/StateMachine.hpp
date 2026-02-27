#pragma once
#include <unordered_map>
#include <string>
 

class GameObject;

 
class State
{
public:
    virtual ~State() = default;

    /// <summary>
    /// Called once when entering this state.
    /// Use for initialization (e.g., start animation, reset timers).
    /// </summary>
    virtual void enter(GameObject* owner) {}

    /// <summary>
    /// Called every frame while in this state.
    /// Main logic goes here.
    /// </summary>
    virtual void update(GameObject* owner, float dt) = 0;

    /// <summary>
    /// Called once when exiting this state.
    /// Use for cleanup (e.g., stop sounds, reset flags).
    /// </summary>
    virtual void exit(GameObject* owner) {}

    /// <summary>
    /// Returns the name of this state for debugging.
    /// </summary>
    virtual const char* getName() const { return "State"; }
};

#pragma once
#include <unordered_map>
#include <string>
#include <memory>

class GameObject;
class State;

/// <summary>
/// Finite State Machine for AI behavior management.
/// Manages state transitions and lifecycle (enter/update/exit).
/// </summary>
class StateMachine
{
public:
    explicit StateMachine(GameObject* owner = nullptr);
    ~StateMachine();

    // Prevent copying (state machines are unique per entity)
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    void setOwner(GameObject* owner);
    GameObject* getOwner() const { return m_owner; }

    // ==================== State Registration ====================

    /// <summary>
    /// Creates and registers a new state with perfect forwarding.
    /// Template version for inline state creation.
    /// </summary>
    /// <example>
    /// fsm->addState<PatrolState>("Patrol", patrolSpeed);
    /// </example>
    template <typename T, typename... Args>
    T* addState(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of<State, T>::value, 
                      "T must inherit from State");
        
        if (hasState(name))
        {
            // Warning: state already exists
            return static_cast<T*>(m_states[name]);
        }

        auto* state = new T(std::forward<Args>(args)...);
        m_states[name] = state;
        return state;
    }

    /// <summary>
    /// Registers an existing state instance.
    /// Takes ownership of the pointer.
    /// </summary>
    State* addState(const std::string& name, State* state);

    State* getState(const std::string& name) const;
    bool hasState(const std::string& name) const;

    // ==================== State Control ====================

    /// <summary>
    /// Sets the initial state WITHOUT calling enter().
    /// Use this only during setup before first update.
    /// For runtime transitions, use changeState().
    /// </summary>
    void setInitialState(const std::string& name);

    /// <summary>
    /// Sets a state that runs every frame regardless of current state.
    /// Useful for health checks, death detection, etc.
    /// </summary>
    void setGlobalState(const std::string& name);

    /// <summary>
    /// Main update loop. Call this every frame.
    /// Executes global state (if any), then current state.
    /// </summary>
    void update(float dt);

    /// <summary>
    /// Transitions to a new state.
    /// Calls exit() on current, then enter() on new state.
    /// </summary>
    void changeState(const std::string& name);

    /// <summary>
    /// Returns to the previous state.
    /// Useful for temporary "blip" states (e.g., react to damage).
    /// </summary>
    void revertToPreviousState();

    // ==================== State Queries ====================

    State* getCurrentState() const { return m_currentState; }
    State* getPreviousState() const { return m_previousState; }
    State* getGlobalState() const { return m_globalState; }

    bool isInState(const std::string& name) const;

private:
    GameObject* m_owner;

    // All registered states (owned by this machine)
    std::unordered_map<std::string, State*> m_states;

    State* m_currentState;
    State* m_previousState;
    State* m_globalState;
};
