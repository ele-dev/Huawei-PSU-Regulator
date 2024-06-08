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
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
std::string sendGetRequest(const std::string &url, const std::string &user, const std::string &password);
void sendPostRequest(const std::string &url, const std::string &user, const std::string &password, const std::string &jsonData);

struct PowerState
{
    short tasmotaPowerCmd;
    short psuAcInputPower;
};


