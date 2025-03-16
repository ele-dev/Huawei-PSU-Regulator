/*
    File: fsm.h
    This class represents the central finite state machine that defines 
    the overall behavior of the control system

    written by Elias Geiger
*/

#pragma once

#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>

#include "opendtu-interface.h"
#include "PsuController.h"
#include "Utils.h"
#include "ModbusClient.h"

using std::this_thread::sleep_for;
using std::chrono::seconds;

// Enumeration for FSM states
enum class State {
    IDLE,
    CHARGING,
    DISCHARGING
};

// Enumeration for FSM events
enum class Event {
    PV_OVERPRODUCTION,
    HIGH_DEMAND,
    BATTERY_FULL,
    BATTERY_LOW
};

// Structure to hold event conditions composed of both value thresholds coupled to time hysteresis
struct EventCondition {
    std::function<bool()> condition;
    std::chrono::seconds hysteresis;
    std::chrono::steady_clock::time_point lastChecked;
    bool conditionMet;

    EventCondition(std::function<bool()> cond, std::chrono::seconds hyst)
        : condition(cond), hysteresis(hyst), lastChecked(std::chrono::steady_clock::now()), conditionMet(false) {}
};

class PVPowerPlantFSM {
public:
    PVPowerPlantFSM(OpenDtuInterface* dtu, PsuController* psu, ModbusClient* powermeter);
    ~PVPowerPlantFSM();

    // process possible events which might trigger a state transition
    void update(GridLoadState gridState, int acInvSupply, float batteryVoltage);

private:
    State currentState;
    std::unordered_map<State, std::unordered_map<Event, State>> transitionTable;
    std::unordered_map<State, std::function<void()>> actionTable;
    std::unordered_map<Event, EventCondition> eventConditions;

    OpenDtuInterface* m_dtu;
    PsuController* m_psu;
    ModbusClient* m_modbusPM;

    // measurement variables
    short m_gridLoad;
    short m_acChargePower;
    int m_acInvToGridPower;
    float m_batteryVoltage;

    void handleEvent(Event event);

    std::string getEventName(Event event);
    std::string getStateName(State state);

    // state entry actions //
    void idleStateEntryAction();
    void chargeStateEntryAction();
    void dischargeStateEntryAction();

    // event trigger conditions //
    bool pvOverproduction();
    bool highDemand();
    bool batteryFull();
    bool batteryLow();

    void psuPowerRegulation();
    float calculateCurrentBasedOnPower(float power, float batteryVoltage) const;
};