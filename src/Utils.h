/*
    File: Utils.h

    written by Elias Geiger
*/

#pragma once

#include <ctime>
#include <sstream>
#include "ConfigFile.h"

extern ConfigFile cfg;

// function prototypes
bool scheduledClose();
float round(float var);
std::string float2String(float var, int decimalCnt);

struct GridLoadState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


