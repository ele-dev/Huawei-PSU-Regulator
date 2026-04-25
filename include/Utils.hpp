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
bool ScheduledClose();
float Round(float value);
std::string Float2String(float value, int decimalDigits);

enum class PowerMeterType {
    TASMOTA,
    SHELLY
};

struct GridLoadState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


