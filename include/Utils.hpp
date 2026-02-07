/*
    File: Utils.hpp
    written by Elias Geiger
*/

#pragma once

#include <ctime>
#include <sstream>

#include "ConfigFile.hpp"
#include "Logger.hpp"

// function prototypes
bool scheduledClose();
float round(float var);
std::string float2String(float var, int decimalCnt);

enum class PowerMeterType {
    TASMOTA,
    SHELLY
};

struct GridLoadState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


