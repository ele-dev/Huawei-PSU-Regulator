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

#include "opendtu-interface.hpp"
#include "PsuController.hpp"
#include "Utils.hpp"
#include "ModbusClient.hpp"

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

class PVPowerPlantFSM 
{
public:
    PVPowerPlantFSM(OpenDtuInterface* dtu, PsuController* psu, ModbusClient* powermeter);
    ~PVPowerPlantFSM();

    // process possible events which might trigger a state transition
    void Update(GridLoadState gridState, int acInvSupply, float batteryVoltage);

private:
    State m_currentState;
    std::unordered_map<State, std::unordered_map<Event, State>> m_transitionTable;
    std::unordered_map<State, std::function<void()>> m_actionTable;
    std::unordered_map<Event, EventCondition> m_eventConditions;

    OpenDtuInterface* m_dtu;
    PsuController* m_psu;
    ModbusClient* m_modbusPM;

    // measurement variables
    short m_gridLoad;
    short m_acChargePower;
    int m_acInvToGridPower;
    float m_batteryVoltage;

    void HandleEvent(Event event);

    std::string GetEventName(Event event);
    std::string GetStateName(State state);

    // state entry actions //
    void IdleStateEntryAction();
    void ChargeStateEntryAction();
    void DischargeStateEntryAction();

    // event trigger conditions //
    bool PvOverproduction();
    bool HighDemand();
    bool BatteryFull();
    bool BatteryLow();

    void PsuPowerRegulation();
    float CalculateCurrentBasedOnPower(float power, float batteryVoltage) const;
};
