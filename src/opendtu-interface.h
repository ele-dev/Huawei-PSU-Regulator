/*
    File: opendtu-interface.h
    This Module provides a interface to opentu HTTP API using curl requests

    written by Elias Geiger
*/

#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

#include "ConfigFile.h"

using json = nlohmann::json;

class OpenDtuInterface
{
public:
    OpenDtuInterface();
    ~OpenDtuInterface();

    // control methods
    void enableDynamicPowerLimiter();
    void disableDynamicPowerLimiter();
    void fetchCurrentState();

    // measurement getters 
    float getBatteryToGridPower() const;
    float getBatteryVoltage() const;
    // ...

private:
    void fetchInitialDPSState();
    std::string sendGetRequest(const std::string &url) const;
    void sendPostRequest(const std::string &url, const std::string &jsonData) const;

    std::string m_address;
    std::string m_user;
    std::string m_password;
    std::string m_batteryInverterId;

    float m_BatteryToGridPower;
    float m_BatteryVoltage;
    bool m_DynamicLimiterEnabled;
    // ...
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);