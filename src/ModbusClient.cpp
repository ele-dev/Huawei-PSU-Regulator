/*
    File: ModbusClient.cpp
    written by Elias Geiger
*/

#include "ModbusClient.h"

// Definitions //

extern Queue<GridLoadState> cmdQueue;
extern PsuController psu;
extern ConfigFile cfg;
extern Logger logger;

ModbusClient::ModbusClient() : m_threadRunning(false) {}

ModbusClient::~ModbusClient() {}

bool ModbusClient::setup(const char* serverIp, const int serverPort) {
    this->m_pollingPeriodTime = cfg.getPowerMeterModbusPollingPeriod();

    // initialize the modbus TCP connection
    this->m_connectionHandle = modbus_new_tcp(serverIp, serverPort);
    if (this->m_connectionHandle == NULL) {
        logger.logMessage(LogLevel::ERROR, "[Modbus] Unable to allocate libmodbus context");
        return false;
    }

    // set auto recovery when errors occur
    int mode = MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL;
    if(modbus_set_error_recovery(this->m_connectionHandle, (modbus_error_recovery_mode)mode) < 0) {
        logger.logMessage(LogLevel::WARNING, "[Modbus] Failed to configure auto recovery");
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Details: " + std::string(modbus_strerror(errno)));
    }

    // set a timeout of 3 seconds to wait for responses
    if(modbus_set_response_timeout(this->m_connectionHandle, 3, 0) < 0) {
        logger.logMessage(LogLevel::WARNING, "[Modbus] Failed to configure response timeout");
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Details: " + std::string(modbus_strerror(errno)));
    }

    // connect to the modbus power meter
    if (modbus_connect(this->m_connectionHandle) == -1) {
        logger.logMessage(LogLevel::ERROR, "[Modbus] " + std::string(modbus_strerror(errno)));
        modbus_free(this->m_connectionHandle);
        return false;
    }

    // configure TCP keep alive for longterm stable connection
    #ifdef USE_TCP_KEEPALIVE_OPTIONS
        int tcpSocket = modbus_get_socket(this->m_connectionHandle);
        if(tcpSocket == -1) {
            logger.logMessage(LogLevel::ERROR, "[Modbus] " + std::string(modbus_strerror(errno)));
            modbus_close(this->m_connectionHandle);
            modbus_free(this->m_connectionHandle);
            return false;
        }

        if(!this->enableTcpKeepalive(tcpSocket)) {
            logger.logMessage(LogLevel::ERROR, "[Modbus] Failed to configure TCP-Keepalive");
            modbus_close(this->m_connectionHandle);
            modbus_free(this->m_connectionHandle);
            return false;
        }
    #endif
    
    // flush before first start of communication
    modbus_flush(this->m_connectionHandle);

    // spawn separate thread to fetch power meter readings in background
    this->m_listenerThread = std::thread([] (ModbusClient* ptr) {
        ptr->m_threadRunning = true;
        logger.logMessage(LogLevel::DEBUG, "[Modbus-thread] listener thread running ...");

        // power meter polling loop
        while(ptr->m_threadRunning)
        {
            // read modbus register from modbus powermeter (e.g. shelly pro 3em)
            float registerValue = ptr->readInputRegisterAsFloat32(SHELLY_POWER_REG_ADDR);
            if(registerValue == INVALID_POWERMETER_READ) {
                continue;
            }

            // round value and convert to short integer
            short powerVal = static_cast<short>(round(registerValue));

            // filter out invalid unrealistic values with a nofication sent along
            if(powerVal < -30000 || powerVal > 20000) {
                logger.logMessage(LogLevel::WARNING, "[Modbus-thread] Received invalid power state value: " + std::to_string(powerVal) + " (ignore)");
                continue;
            }

            // compose a power state object out of the new command and the current AC input power of the PSU
            GridLoadState pState;
            pState.tasmotaPowerCmd = powerVal;
            pState.psuAcInputPower = static_cast<short>(psu.getCurrentInputPower());
            
            // put new state on the command queue for processing
            cmdQueue.push(pState);

            // idle a bit to prevent buisy waiting
            sleep_for(milliseconds(ptr->m_pollingPeriodTime));
        }

        logger.logMessage(LogLevel::DEBUG, "[Modbus-thread] closeup --> finish thread now");
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
    logger.logMessage(LogLevel::INFO, "[Modbus] Increased powermeter polling rate for regulation");
}

void ModbusClient::decreaseModbusPollingRate() {
    this->m_pollingPeriodTime = 4000;
    logger.logMessage(LogLevel::INFO, "[Modbus] Decreased powermeter polling rate");
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
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Failed to enable SO_KEEPALIVE");
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepaliveIdle, sizeof(keepaliveIdle));
    if(result < 0) {
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Failed to set TCP_KEEPIDLE");
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepaliveProbeIntv, sizeof(keepaliveProbeIntv));
    if(result < 0) {
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Failed to set TCP_KEEPINTVL");
        return false;
    }

    result = setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepAliveProbeCnt, sizeof(keepAliveProbeCnt));
    if(result < 0) {
        logger.logMessage(LogLevel::DEBUG, "[Modbus] Failed to set TCP_KEEPCNT");
        return false;
    }

    return true;
}

float ModbusClient::readInputRegisterAsFloat32(int startRegAddr) const {
    const int registerCount = 2;     // To store 2 input registers (each register is 16 bits)
    uint16_t tab_reg[registerCount];     
    int statusCode = 0;

    // attempt to read the 2 registers 
    statusCode = modbus_read_input_registers(this->m_connectionHandle, startRegAddr, registerCount, tab_reg);
    if(statusCode == -1) {
        // check if the error is related to disconnection or protocol failure
        if (errno == ECONNRESET || errno == EPIPE || errno == EBADF) {
            logger.logMessage(LogLevel::WARNING, "[Modbus] Connection lost. Attempting to reconnect ...");
            logger.logMessage(LogLevel::DEBUG, "[Modbus] Details: " + std::string(modbus_strerror(errno)));

            // attempt to restablish a stable TCP connection
            modbus_close(this->m_connectionHandle);
            sleep_for(milliseconds(3000));
            if (modbus_connect(this->m_connectionHandle) == -1) {
                logger.logMessage(LogLevel::ERROR, "[Modbus] Reconnection attempt failed");
                logger.logMessage(LogLevel::DEBUG, "[Modbus] Details: " + std::string(modbus_strerror(errno)));
            } else {
                logger.logMessage(LogLevel::INFO, "[Modbus] Reconnected to modbus powermeter");
            }
        }
        return INVALID_POWERMETER_READ;
    }

    float value = modbus_get_float_abcd(tab_reg);

    return value;
}