/*
    File: Utils.h

    written by Elias Geiger
*/

#pragma once

#include <ctime>
#include "ConfigFile.h"

extern ConfigFile cfg;

// function prototypes
bool scheduledClose();
float round(float var);

struct GridLoadState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


