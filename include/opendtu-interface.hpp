/*
    File: opendtu-interface.hpp
    This Module provides a interface to opentu HTTP API using curl requests
    
    written by Elias Geiger
*/

#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "Utils.hpp"

#define HTTP_TIMEOUT_SEC 2

class OpenDtuInterface
{
public:
    OpenDtuInterface();
    ~OpenDtuInterface();

    // control methods
    void EnableDynamicPowerLimiter();
    void DisableDynamicPowerLimiter();
    void FetchCurrentState();

    // measurement getters 
    float GetBatteryToGridPower() const;
    float GetBatteryVoltage() const;
    // ...

private:
    void SetupCurlHandles();
    void FetchInitialDPLState();
    std::string SendGetRequest(const std::string &url) const;
    void SendPostRequest(const std::string &url, const std::string &jsonData) const;

    CURL *m_curl_get_handle;
    CURL *m_curl_post_handle;
    // bool m_writeCallbackDone;

    std::string m_address;
    std::string m_http_credentials;
    std::string m_batteryInverterId;
    std::string m_startDischargeVoltage;
    std::string m_stopDischargeVoltage;

    float m_BatteryToGridPower;
    float m_BatteryVoltage;
    bool m_DynamicLimiterEnabled;
    // ...
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
