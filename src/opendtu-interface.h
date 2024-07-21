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
#include <thread>
#include <chrono>

#include "Utils.h"

using json = nlohmann::json;

#define HTTP_TIMEOUT_SEC 2

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
    void setupCurlHandles();
    void fetchInitialDPLState();
    std::string sendGetRequest(const std::string &url) const;
    void sendPostRequest(const std::string &url, const std::string &jsonData) const;

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