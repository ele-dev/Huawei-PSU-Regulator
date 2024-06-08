/*
    File: Utils.h

    written by Elias Geiger
*/

#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp> 
#include <ctime>
#include "ConfigFile.h"

extern ConfigFile cfg;

// function prototypes
bool scheduledClose();
std::string sendGetRequest(const std::string &url, const std::string &user, const std::string &password);
void sendPostRequest(const std::string &url, const std::string &user, const std::string &password, const std::string &jsonData);

struct PowerState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


