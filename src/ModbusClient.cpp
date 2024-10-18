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

    // initialize the modbus TCP connection
    this->m_connectionHandle = modbus_new_tcp(serverIp, serverPort);
    if (this->m_connectionHandle == NULL) {
        std::cerr << "[MODBUS] Error: Unable to allocate libmodbus context" << std::endl;
        return false;
    }

    // connect to the modbus power meter
    if (modbus_connect(this->m_connectionHandle) == -1) {
        std::cerr << "[MODBUS] Error: " << modbus_strerror(errno) << std::endl;
        modbus_free(this->m_connectionHandle);
        return false;
    }

    // configure TCP keep alive for longterm stable connection
    int tcpSocket = modbus_get_socket(this->m_connectionHandle);
    if(tcpSocket == -1) {
        std::cerr << "[MODBUS] Error: " << modbus_strerror(errno) << std::endl;
        modbus_close(this->m_connectionHandle);
        modbus_free(this->m_connectionHandle);
        return false;
    }

    if(!this->enableTcpKeepalive(tcpSocket)) {
        std::cerr << "[MODBUS] Error: Failed to configure TCP-Keepalive!" << std::endl;
        modbus_close(this->m_connectionHandle);
        modbus_free(this->m_connectionHandle);
        return false;
    }
    
    // flush before first start of communication
    modbus_flush(this->m_connectionHandle);

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
                // flush and wait a few seconds before trying again 
                modbus_flush(ptr->m_connectionHandle);
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
    modbus_close(this->m_connectionHandle);
    modbus_free(this->m_connectionHandle);
}

void ModbusClient::increaseModbusPollingRate() {
    this->m_pollingPeriodTime = cfg.getPowerMeterModbusPollingPeriod();
    std::cout << "[MODBUS] Increased powermeter polling rate for regulation" << std::endl;
}

void ModbusClient::decreaseModbusPollingRate() {
    this->m_pollingPeriodTime = 4000;
    std::cout << "[MODBUS] Decreased powermeter polling rate" << std::endl;
}

bool ModbusClient::enableTcpKeepalive(int sock) {
    // define keep alive parameters
    const int keepalive = 1;
    const int keepaliveIdle = 60;
    const int keepaliveProbeIntv = 10;
    const int keepAliveProbeCnt = 5;

    // Do the neccessary socket configuration steps
    int result = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    if(result < 0) {
        std::cerr << "[MODBUS] Error enabling SO_KEEPALIVE" << std::endl;
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepaliveIdle, sizeof(keepaliveIdle));
    if(result < 0) {
        std::cerr << "[MODBUS] Error setting TCP_KEEPIDLE" << std::endl;
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepaliveProbeIntv, sizeof(keepaliveProbeIntv));
    if(result < 0) {
        std::cerr << "[MODBUS] Error setting TCP_KEEPINTVL" << std::endl;
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepAliveProbeCnt, sizeof(keepAliveProbeCnt));
    if(result < 0) {
        std::cerr << "[MODBUS] Error setting TCP_KEEPCNT" << std::endl;
        return false;
    }

    return true;
}

float ModbusClient::readInputRegisterAsFloat32(int startRegAddr) const {
    uint16_t tab_reg[2];     // To store 2 input registers (each register is 16 bits)
    int statusCode = 0;

    // attempt to read the 2 registers 
    statusCode = modbus_read_input_registers(this->m_connectionHandle, startRegAddr, 2, tab_reg);
    if(statusCode == -1) {
        std::cerr << "Failed to read: " << modbus_strerror(errno) << std::endl;
        return INVALID_POWERMETER_READ;
    }

    float value = modbus_get_float_abcd(tab_reg);

    return value;
}