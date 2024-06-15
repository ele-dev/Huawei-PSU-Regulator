/*
    File: fsm.cpp
    written by Elias Geiger
*/

#include "fsm.h"

PVPowerPlantFSM::PVPowerPlantFSM(OpenDtuInterface* dtu, PsuController* psu)
{
    // init static measurements
    m_gridLoad = 0;
    m_acChargePower = 0;
    m_acInvToGridPower = 0;

    // store dtu & psu references internally
    m_dtu = dtu;
    m_psu = psu;

    // initial state is IDLE
    currentState = State::IDLE;

    // transition table defines available transitions for all states
    transitionTable = {
        {State::IDLE, {{Event::PV_OVERPRODUCTION, State::CHARGING}, {Event::HIGH_DEMAND, State::DISCHARGING}}},
        {State::CHARGING, {{Event::BATTERY_FULL, State::IDLE}, {Event::HIGH_DEMAND, State::DISCHARGING}}},
        {State::DISCHARGING, {{Event::BATTERY_LOW, State::IDLE}, {Event::PV_OVERPRODUCTION, State::CHARGING}}}
    };

    // action table defines task to be executed just after a transition into a specific state
    actionTable = {
        {State::IDLE, std::bind(&PVPowerPlantFSM::idleStateEntryAction, this)},
        {State::CHARGING, std::bind(&PVPowerPlantFSM::chargeStateEntryAction, this)},
        {State::DISCHARGING, std::bind(&PVPowerPlantFSM::dischargeStateEntryAction, this)}
    };

    // event conditions defined by a boolean expression along with a time hysteresis
    eventConditions = {
        {Event::PV_OVERPRODUCTION, EventCondition(std::bind(&PVPowerPlantFSM::pvOverproduction, this), std::chrono::seconds(20))},
        {Event::HIGH_DEMAND, EventCondition(std::bind(&PVPowerPlantFSM::highDemand, this), std::chrono::seconds(15))},
        {Event::BATTERY_FULL, EventCondition(std::bind(&PVPowerPlantFSM::batteryFull, this), std::chrono::seconds(200))},
        {Event::BATTERY_LOW, EventCondition(std::bind(&PVPowerPlantFSM::batteryLow, this), std::chrono::seconds(200))}
    };
}

PVPowerPlantFSM::~PVPowerPlantFSM() 
{
    m_dtu = nullptr;
}

void PVPowerPlantFSM::update(GridLoadState gridState, int acInvSupply)
{
    // first update internal measurement variables
    m_gridLoad = gridState.tasmotaPowerCmd;
    m_acChargePower = gridState.psuAcInputPower;
    m_acInvToGridPower = acInvSupply;

    for (auto &[event, condition] : eventConditions)
    {
        if (condition.condition())
        {
            auto now = std::chrono::steady_clock::now();
            if (!condition.conditionMet)
            {
                condition.lastChecked = now;
                condition.conditionMet = true;
            }
            else if (std::chrono::duration_cast<std::chrono::seconds>(now - condition.lastChecked) >= condition.hysteresis)
            {
                handleEvent(event);
            }
        }
        else
        {
            condition.conditionMet = false;
        }
    }

    // do state dependant non-blocking tasks
    switch(currentState)
    {
        case State::IDLE:
        {
            // wait a short amount of time
            sleep_for(seconds(2));
            break;
        }

        case State::CHARGING:
        {
            // run one loop iteration of psu power regulator ...
            psuPowerRegulation();
            break;
        }

        case State::DISCHARGING:
        {
            // do nothing (DPL from opendtu does the regulation for discharge)
            break;
        }

        default:
        {
            std::cerr << "[FSM] Current State is undefined!" << std::endl;
            break;
        }
    }
}

// central event handler for safe state transitions
void PVPowerPlantFSM::handleEvent(Event event) {
    auto transitions = transitionTable[currentState];
    if (transitions.find(event) != transitions.end()) {

        // transition into following state
        std::cout << "[FSM] !>> Event: " << getEventName(event) << std::endl;
        currentState = transitions[event];

        // execute state entry action
        actionTable[currentState]();
    } else {
        std::cout << "Event (" << getEventName(event) << ") not defined for current state (" << getStateName(currentState) << ").\n";
    }
}

// helper method to get a string representation of events
std::string PVPowerPlantFSM::getEventName(Event event) 
{
    switch(event) {
        case Event::PV_OVERPRODUCTION: return "PV Overprodution";
        case Event::HIGH_DEMAND: return "High Demand";
        case Event::BATTERY_FULL: return "Battery FUll";
        case Event::BATTERY_LOW: return "Batter Low";
        default: return "unknown";
    }
}

// helper method to get a string representation of states
std::string PVPowerPlantFSM::getStateName(State state) 
{
    switch(state) {
        case State::IDLE: return "IDLE";
        case State::CHARGING: return "CHARGING";
        case State::DISCHARGING: return "DISCHARGING";
        default: return "unknown";
    }
}

// state entry actions (executed directly when entering new state) //
void PVPowerPlantFSM::idleStateEntryAction()
{
    std::cout << "[FSM] --> Entering Idle state ..." << std::endl;
}

void PVPowerPlantFSM::chargeStateEntryAction() 
{
    std::cout << "[FSM] --> Entering Charging state ..." << std::endl;
    m_dtu->disableDynamicPowerLimiter();

    // activate the psu regulator for adaptive charging
    // ...
}

void PVPowerPlantFSM::dischargeStateEntryAction() 
{
    std::cout << "[FSM] --> Entering Discharging state ..." << std::endl;
    // pause the psu regulator before starting the DPL

    m_dtu->enableDynamicPowerLimiter();
}

// event conditions //
bool PVPowerPlantFSM::pvOverproduction()
{
    // demand is satisfied and inverter is not supplying additional power from battery
    if(m_gridLoad < 0 && m_acInvToGridPower == 0) {
        return true;
    }
    return false;
}

bool PVPowerPlantFSM::highDemand()
{
    // demand is high and AC charger is not charging 
    if(m_gridLoad > 0 && m_acChargePower == 0 && m_acInvToGridPower == 0) {
        return true;
    }
    return false;
}

bool PVPowerPlantFSM::batteryFull()
{
    return false;
}

bool PVPowerPlantFSM::batteryLow()
{
    // demand is still high but inverter doesn't supply power (because battery is empty)
    if(m_gridLoad > 20 && m_acInvToGridPower == 0) {
        return true;
    }
    return false;
}

void PVPowerPlantFSM::psuPowerRegulation() 
{
    short error = 0;

    // calculate error (absolute difference from target value)
    // don't try to compensate for very small errors
    error = cfg.getTargetGridPower() - m_gridLoad;
    std::cout << "[Regulator] Processing received power state: grid-load = " 
                << m_gridLoad << "W, deviation = " 
                << error << "W, AC-charge = "
                << m_acChargePower << "W" << std::endl;

    if(abs(error) < cfg.getRegulatorErrorThreshold()) {
        return;
    }
    short powerCmd = m_acChargePower + error;

    // set bounds for allowed power commands (min and max)
    if(powerCmd > cfg.getMaxChargePower()) {
        powerCmd = cfg.getMaxChargePower();
    }

    if(powerCmd < cfg.getMinChargePower()) {
        powerCmd = 0;
    }

    // translate power command into max current command. use current output voltage for calculation
    float maxCurrentCmd = this->calculateCurrentBasedOnPower(static_cast<float>(powerCmd), m_psu->getCurrentOutputVoltage());

    // send max current command to the PSU and idle a short time 
    m_psu->setMaxCurrent(maxCurrentCmd, false);

    std::cout << "[Regulator] Target AC charger power --> " << powerCmd << "W" << std::endl;

    sleep_for(milliseconds(cfg.getRegulatorIdleTime()));
}

float PVPowerPlantFSM::calculateCurrentBasedOnPower(float power, float batteryVoltage) const 
{
    // Determine expected AC/DC conversion efficiency based on power command
    float eff = 0.0f;
    if(power >= 1 && power < 461) {
        eff = 0.88f;
    } else if(power >= 461 && power < 704) {
        eff = 0.937f;
    } else if(power >= 704 && power < 1050) {
        eff = 0.952f;
    } else if(power >= 1050) {
        eff = 0.96f;
    }

    // calculate and round the current 
    float result = round(0.9876f * eff * power / batteryVoltage);

    return result;
}