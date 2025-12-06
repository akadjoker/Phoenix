#include "StateMachineComponent.hpp"
#include "GameObject.hpp"

StateMachineComponent::StateMachineComponent()
    : m_fsm(nullptr)
{
}

void StateMachineComponent::attach()
{
   m_fsm.setOwner(getOwner());
}

void StateMachineComponent::update(float deltaTime)
{
    m_fsm.update(deltaTime);
}
