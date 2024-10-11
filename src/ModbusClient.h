#pragma once

#include <modbus/modbus.h>
#include <cstdint>
#include <cstring>
#include <thread>

#include "Queue.cpp"
#include "PsuController.h"
#include "Utils.h"

#define SHELLY_POWER_REG_ADDR 1014
#define SHELLY_MODBUS_PORT 502

using std::chrono::seconds;
using std::this_thread::sleep_for;

class ModbusClient 
{
    private:
        modbus_t *connectionHandle;       // connection handle
        bool ready;

        std::thread m_listenerThread;
        std::atomic<bool> m_threadRunning;

    public:
        ModbusClient();
        ~ModbusClient();

        bool setup(const char* serverIp, const int serverPort);
        void closeup();

        float readInputRegisterAsFloat32(int startRegAddr) const;

    private:
        bool isReady() const;
        int readInputRegisterAsInt16(int startRegAddr) const;
};