/*
    File: ModbusClient.h
    This module provides functionality for integration of Modbus/TCP enabled 
    3 phase powermeters such as the Shelly Pro 3em

    written by Elias Geiger
*/

#pragma once

#include <modbus/modbus.h>
#include <cstdint>
#include <cstring>
#include <thread>

#include "Queue.cpp"
#include "PsuController.h"
#include "Utils.h"

#define SHELLY_POWER_REG_ADDR 1014
#define INVALID_POWERMETER_READ -9999.9f

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

class ModbusClient 
{
    private:
        modbus_t *connectionHandle;       // connection handle

        std::thread m_listenerThread;
        std::atomic<bool> m_threadRunning;
        std::atomic<int> m_pollingPeriodTime;

    public:
        ModbusClient();
        ~ModbusClient();

        bool setup(const char* serverIp, const int serverPort);
        void closeup();

        // setter functions for in-/decreasing modbus polling rate from outside
        void increaseModbusPollingRate();
        void decreaseModbusPollingRate();

    private:
        float readInputRegisterAsFloat32(int startRegAddr) const;
};