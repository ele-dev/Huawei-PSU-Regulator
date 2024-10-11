/*
    File: ModbusClient.cpp
    written by Elias Geiger
*/

#include "ModbusClient.h"

// Definitions //

extern Queue<GridLoadState> cmdQueue;
extern PsuController psu;

ModbusClient::ModbusClient() {}

ModbusClient::~ModbusClient() {}

bool ModbusClient::setup(const char* serverIp, const int serverPort) {
    // Initialize the Modbus TCP connection
    this->connectionHandle = modbus_new_tcp(serverIp, serverPort);
    if (this->connectionHandle == NULL) {
        std::cerr << "Unable to allocate libmodbus context" << std::endl;
        return false;
    }

    // Connect to the Modbus server
    if (modbus_connect(this->connectionHandle) == -1) {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
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
            // read modbus register from shelly 
            float registerValue = ptr->readInputRegisterAsFloat32(SHELLY_POWER_REG_ADDR);

            // round value and convert to short integer
            short powerVal = static_cast<short>(round(registerValue));

            // filter out invalid unrealistic values
            if(powerVal < -30000 || powerVal > 20000) {
                std::cerr << "[MODBUS-thread] Received invalid power state value: " << powerVal << " (ignore)" << std::endl;
                continue;
            }
            else {
                std::cout << "[MODBUS-thread] Fetched Powermeter: " << powerVal << "W" << std::endl;
            }

            // compose a power state object out of the new command and the current AC input power of the PSU
            GridLoadState pState;
            pState.tasmotaPowerCmd = powerVal;
            pState.psuAcInputPower = static_cast<short>(psu.getCurrentInputPower());
            
            // put new state on the command queue for processing
            cmdQueue.push(pState);

            // idle a bit to prevent buisy waiting
            sleep_for(seconds(1));
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

float ModbusClient::readInputRegisterAsFloat32(int startRegAddr) const {
    uint16_t tab_reg[2];     // To store 2 input registers (each register is 16 bits)
    int statusCode = 0;

    // attempt to read the 2 registers 
    statusCode = modbus_read_input_registers(this->connectionHandle, startRegAddr, 2, tab_reg);
    if(statusCode == -1) {
        std::cerr << "Failed to read: " << modbus_strerror(errno) << std::endl;
        return -1.0f;
    }

    // combine into one 32-bit register (big-endian byte order)
    uint32_t raw_value = (tab_reg[0] << 16) | tab_reg[1];

    // parse content into float variable according to IEEE-754 standard
    float value;
    std::memcpy(&value, &raw_value, sizeof(value));

    return value;
}

int ModbusClient::readInputRegisterAsInt16(int startRegAddr) const {
    return 0;
}
