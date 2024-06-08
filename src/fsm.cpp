#include "fsm.h"

// definitions //

FSM::FSM()
{
    // Initialize the current state
    currentState = State::START;

    // Define state transition table
    transitionTable = {
        { State::START, {{Event::EVENT1, State::STATE1}, {Event::EVENT2, State::STATE2}}},
        { State::STATE1, {{Event::EVENT1, State::STATE2}, {Event::EVENT2, State::END}}},
        { State::STATE2, {{Event::EVENT1, State::END}, {Event::EVENT2, State::START}}},
        { State::END, {}}
    };

    // Define state action table
    actionTable = {
        { State::START, []() { std::cout << "Entering START state.\n"; }},
        { State::STATE1, []() { std::cout << "Entering STATE1 state.\n"; }},
        { State::STATE2, []() { std::cout << "Entering STATE2 state.\n"; }},
        { State::END, []() { std::cout << "Entering END state.\n"; }}
    };
}

void FSM::handleEvent(Event event) {
    auto transitions = transitionTable[currentState];
    if (transitions.find(event) != transitions.end()) {
        currentState = transitions[event];
        actionTable[currentState]();
    } else {
        std::cout << "Invalid event for current state.\n";
    }
}