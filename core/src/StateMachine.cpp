#include "pch.h"
#include "StateMachine.hpp"
#include "StateMachine.hpp"
#include <cassert>
 
StateMachine::StateMachine(GameObject* owner)
    : m_owner(owner)
    , m_currentState(nullptr)
    , m_previousState(nullptr)
    , m_globalState(nullptr)
{
}

StateMachine::~StateMachine()
{
    
    for (auto& pair : m_states)
    {
        delete pair.second;
    }
    m_states.clear();
}

void StateMachine::setOwner(GameObject* owner)
{
    m_owner = owner;
}

State* StateMachine::addState(const std::string& name, State* state)
{
    if (!state) return nullptr;

    if (hasState(name))
    {
        // Warning: replacing existing state
        delete m_states[name];
    }

    m_states[name] = state;
    return state;
}

State* StateMachine::getState(const std::string& name) const
{
    auto it = m_states.find(name);
    return (it != m_states.end()) ? it->second : nullptr;
}

bool StateMachine::hasState(const std::string& name) const
{
    return m_states.find(name) != m_states.end();
}

void StateMachine::setInitialState(const std::string& name)
{
    m_currentState = getState(name);
    if (m_currentState && m_owner)
    {
        m_currentState->enter(m_owner);  
    }
}

void StateMachine::setGlobalState(const std::string& name)
{
    m_globalState = getState(name);
}

void StateMachine::update(float dt)
{
    if (!m_owner) return;

    // Global state runs always
    if (m_globalState)
        m_globalState->update(m_owner, dt);

    // Current state logic
    if (m_currentState)
        m_currentState->update(m_owner, dt);
}

void StateMachine::changeState(const std::string& name)
{
    if (!m_owner) return; 

    State* newState = getState(name);
    if (!newState || newState == m_currentState)
        return; // No change needed

    // Save history
    m_previousState = m_currentState;

    // Exit current
    if (m_currentState)
        m_currentState->exit(m_owner);

    // Switch
    m_currentState = newState;

    // Enter new
    m_currentState->enter(m_owner);
}

void StateMachine::revertToPreviousState()
{
    if (!m_previousState || !m_owner) return;

    // Exit current
    if (m_currentState)
        m_currentState->exit(m_owner);

    // Swap current <-> previous
    State* temp = m_currentState;
    m_currentState = m_previousState;
    m_previousState = temp;

    // Enter restored state
    m_currentState->enter(m_owner);
}

bool StateMachine::isInState(const std::string& name) const
{
    State* state = getState(name);
    return state && (m_currentState == state);
}
