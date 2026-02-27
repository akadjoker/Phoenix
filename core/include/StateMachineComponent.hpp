#pragma once

#include "Component.hpp"
#include "StateMachine.hpp"

class StateMachineComponent : public Component
{
public:
    StateMachineComponent();
      ~StateMachineComponent() override = default;

    const char* getTypeName() const override { return "StateMachineComponent"; }

    void attach() override;
    void update(float deltaTime) override;

    StateMachine& getStateMachine() { return m_fsm; }
    const StateMachine& getStateMachine() const { return m_fsm; }

private:
    StateMachine m_fsm;
};
