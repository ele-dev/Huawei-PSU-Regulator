/*
    File: ModbusClient.cpp
    written by Elias Geiger
*/

#include "ModbusClient.h"

// Definitions //

extern Queue<GridLoadState> cmdQueue;
extern PsuController psu;
extern ConfigFile cfg;

ModbusClient::ModbusClient() : m_threadRunning(false) {}

ModbusClient::~ModbusClient() {}

bool ModbusClient::setup(const char* serverIp, const int serverPort) {
    this->m_pollingPeriodTime = cfg.getPowerMeterModbusPollingPeriod();

    // Initialize the Modbus TCP connection
    this->connectionHandle = modbus_new_tcp(serverIp, serverPort);
    if (this->connectionHandle == NULL) {
        std::cerr << "[MODBUS] Error: Unable to allocate libmodbus context" << std::endl;
        return false;
    }

    // Connect to the Modbus server
    if (modbus_connect(this->connectionHandle) == -1) {
        std::cerr << "[MODBUS] Error: " << modbus_strerror(errno) << std::endl;
        modbus_free(this->connectionHandle);
        return false;
    }

    // spawn separate thread to fetch power meter readings in background
    this->m_listenerThread = std::thread([] (ModbusClient* ptr) {
        ptr->m_threadRunning = true;
        std::cout << "[MODBUS-thread] listener thread running ..." << std::endl;

        // power meter polling loop
        while(ptr->m_threadRunning)
        {
            // read modbus register from modbus powermeter (e.g. shelly pro 3em)
            float registerValue = ptr->readInputRegisterAsFloat32(SHELLY_POWER_REG_ADDR);
            if(registerValue == INVALID_POWERMETER_READ) {
                // wait a few seconds before trying again 
                sleep_for(milliseconds(4000));
                continue;
            }

            // round value and convert to short integer
            short powerVal = static_cast<short>(round(registerValue));

            // filter out invalid unrealistic values with a nofication sent along
            if(powerVal < -30000 || powerVal > 20000) {
                std::cerr << "[MODBUS-thread] Received invalid power state value: " << powerVal << " (ignore)" << std::endl;
                continue;
            }
            
            // std::cout << "[MODBUS-thread] Fetched Powermeter: " << powerVal << "W" << std::endl;

            // compose a power state object out of the new command and the current AC input power of the PSU
            GridLoadState pState;
            pState.tasmotaPowerCmd = powerVal;
            pState.psuAcInputPower = static_cast<short>(psu.getCurrentInputPower());
            
            // put new state on the command queue for processing
            cmdQueue.push(pState);

            // idle a bit to prevent buisy waiting
            sleep_for(milliseconds(ptr->m_pollingPeriodTime));
        }

        std::cout << "[MODBUS-thread] closeup --> finish thread now" << std::endl;
    }, 
    this);

    return true;
}

void ModbusClient::closeup() {
    // signal listener thread to stop
    m_threadRunning = false;

    // wait for thread finish
    m_listenerThread.join();

    // Close the connection and free the Modbus context
    modbus_close(this->connectionHandle);
    modbus_free(this->connectionHandle);
}

void ModbusClient::increaseModbusPollingRate() {
    this->m_pollingPeriodTime = cfg.getPowerMeterModbusPollingPeriod();
    std::cout << "[MODBUS] Increased powermeter polling rate for regulation" << std::endl;
}

void ModbusClient::decreaseModbusPollingRate() {
    this->m_pollingPeriodTime = 4000;
    std::cout << "[MODBUS] Decreased powermeter polling rate" << std::endl;
}

float ModbusClient::readInputRegisterAsFloat32(int startRegAddr) const {
    uint16_t tab_reg[2];     // To store 2 input registers (each register is 16 bits)
    int statusCode = 0;

    // attempt to read the 2 registers 
    statusCode = modbus_read_input_registers(this->connectionHandle, startRegAddr, 2, tab_reg);
    if(statusCode == -1) {
        std::cerr << "Failed to read: " << modbus_strerror(errno) << std::endl;
        return INVALID_POWERMETER_READ;
    }

    // combine into one 32-bit register (big-endian byte order)
    uint32_t raw_value = (tab_reg[0] << 16) | tab_reg[1];

    // parse content into float variable according to IEEE-754 standard
    float value;
    std::memcpy(&value, &raw_value, sizeof(value));

    return value;
}