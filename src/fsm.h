#include <iostream>
#include <unordered_map>
#include <functional>

// Enumeration for FSM states
enum class State {
    START,
    STATE1,
    STATE2,
    END
};

// Enumeration for FSM events
enum class Event {
    EVENT1,
    EVENT2,
    END
};

class FSM {
public:
    FSM();
    void handleEvent(Event event);

private:
    State currentState;
    std::unordered_map<State, std::unordered_map<Event, State>> transitionTable;
    std::unordered_map<State, std::function<void()>> actionTable;
};