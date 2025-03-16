/*
    File: opendtu-interface.cpp
    written by Elias Geiger
*/

#include "opendtu-interface.h"

extern ConfigFile cfg;
extern Logger logger;

OpenDtuInterface::OpenDtuInterface() : m_BatteryToGridPower(0.0f), m_BatteryVoltage(40.0f) 
{
    // intit class internal parameters
    m_address = cfg.getOpenDtuHost();
    m_http_credentials = cfg.getOpenDtuAdminUser() + ":" + cfg.getOpenDtuAdminPassword();
    m_batteryInverterId = cfg.getOpenDtuBatteryInverterId();
    m_startDischargeVoltage = float2String(cfg.getOpenDtuStartDischargeVoltage(), 1);
    m_stopDischargeVoltage = float2String(cfg.getOpenDtuStopDischargeVoltage(), 1);
    // m_writeCallbackDone = false;

    // setup CURL
    setupCurlHandles();
    fetchInitialDPLState();
}

OpenDtuInterface::~OpenDtuInterface() 
{
    // close CURL handles
    // m_writeCallbackDone = true;
    curl_easy_cleanup(m_curl_get_handle);
    curl_easy_cleanup(m_curl_post_handle);
}

void OpenDtuInterface::enableDynamicPowerLimiter() 
{
    // setup url and json payload to enable DPL via POST request
    std::string url = "http://" + m_address + "/api/powerlimiter/config";
    std::string jsonStr = "data={\"enabled\":true,\"verbose_logging\":false,\"solar_passthrough_enabled\":false,\"is_inverter_behind_powermeter\":true,\"inverter_id\":0,\"inverter_channel_id\":0,\"target_power_consumption\":5,\"target_power_consumption_hysteresis\":5,\"lower_power_limit\":30,\"upper_power_limit\":800,\"battery_soc_start_threshold\":80,\"battery_soc_stop_threshold\":20,\"voltage_start_threshold\":" + m_startDischargeVoltage + ",\"voltage_stop_threshold\":" + m_stopDischargeVoltage + ",\"voltage_load_correction_factor\":0.0015,\"inverter_restart_hour\":0}";

    // send the POST request
    logger.logMessage(LogLevel::INFO, "[OpenDTU] Request to enable DPL ...");
    sendPostRequest(url, jsonStr);
    m_DynamicLimiterEnabled = true;
}

void OpenDtuInterface::disableDynamicPowerLimiter()
{
    // setup url and json payload to disable DPL via POST request
    std::string url = "http://" + m_address + "/api/powerlimiter/config";    
    std::string jsonStr = "data={\"enabled\":false,\"verbose_logging\":false,\"solar_passthrough_enabled\":false,\"is_inverter_behind_powermeter\":true,\"inverter_id\":0,\"inverter_channel_id\":0,\"target_power_consumption\":5,\"target_power_consumption_hysteresis\":5,\"lower_power_limit\":30,\"upper_power_limit\":800,\"battery_soc_start_threshold\":80,\"battery_soc_stop_threshold\":20,\"voltage_start_threshold\":49.0,\"voltage_stop_threshold\":48.3,\"voltage_load_correction_factor\":0.0015,\"inverter_restart_hour\":0}";

    // send the POST request
    logger.logMessage(LogLevel::INFO, "[OpenDTU] Request to disable DPL ...");
    sendPostRequest(url, jsonStr);
    m_DynamicLimiterEnabled = false;
}

void OpenDtuInterface::fetchCurrentState() 
{
    static bool lastFetchFailed = false;

    // send HTTP GET request for live status data of battery inverter
    std::string url = "http://" + m_address + "/api/livedata/status?inv=" + m_batteryInverterId;
    std::string response = sendGetRequest(url);

    if(response == "failed") {
        lastFetchFailed = true;
        logger.logMessage(LogLevel::WARNING, "[OpenDTU] Failed to fetch current inverter status");
        return;
    }

    if(lastFetchFailed) {
        lastFetchFailed = false;
        logger.logMessage(LogLevel::INFO, "[OpenDTU] Successfully fetched inverter status after connection issues");
    }

    // decode response and extract measurements: inverterPower, batteryVoltage, etc.
    json jsonData = json::parse(response);
    auto ch1 = jsonData["inverters"][0]["DC"]["0"]["Voltage"]["v"];
    auto ch2 = jsonData["inverters"][0]["DC"]["1"]["Voltage"]["v"];
    m_BatteryVoltage = (static_cast<float>(ch1) + static_cast<float>(ch2)) / 2.0;    // take average between both channels
    auto var1 = jsonData["total"]["Power"]["v"];
    m_BatteryToGridPower =  static_cast<float>(var1);
    // ...
}

// measurement getters //

float OpenDtuInterface::getBatteryToGridPower() const {
    return this->m_BatteryToGridPower;
}

float OpenDtuInterface::getBatteryVoltage() const {
    return this->m_BatteryVoltage;
}

// ...

void OpenDtuInterface::fetchInitialDPLState()
{
    // send second GET request to fetch DPL state (on or off)
    std::string url = "http://" + m_address + "/api/powerlimiter/status";
    std::string response = sendGetRequest(url);

    if(response == "failed") {
        logger.logMessage(LogLevel::WARNING, "[OpenDTU] Failed to fetch initial inverter status");
        return;
    }
    json jsonData = json::parse(response);
    m_DynamicLimiterEnabled = jsonData["enabled"];
}

// private helper methods //

void OpenDtuInterface::setupCurlHandles()
{
    // init curl handle for GET requests
    m_curl_get_handle = curl_easy_init();
    if(m_curl_get_handle) {
        curl_easy_setopt(m_curl_get_handle, CURLOPT_USERPWD, m_http_credentials.c_str());
        curl_easy_setopt(m_curl_get_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(m_curl_get_handle, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SEC);
    }

    // init curl handle for POST requests
    m_curl_post_handle = curl_easy_init();
    if(m_curl_post_handle) {
        curl_easy_setopt(m_curl_post_handle, CURLOPT_USERPWD, m_http_credentials.c_str());
        curl_easy_setopt(m_curl_get_handle, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SEC);
    }
}

// callback function for GET request (must remain small and non-blocking) 
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) 
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// function for sending GET request with authentication
std::string OpenDtuInterface::sendGetRequest(const std::string &url) const
{
    CURLcode res;
    std::string readBuffer = "";

    curl_easy_setopt(m_curl_get_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl_get_handle, CURLOPT_WRITEDATA, &readBuffer);

    // perform the request
    res = curl_easy_perform(m_curl_get_handle);

    // check for errors
    if(res == CURLE_COULDNT_CONNECT) {
        logger.logMessage(LogLevel::ERROR, "[OpenDTU] Could not connect via HTTP");
        return "failed";
    }

    if(res == CURLE_OPERATION_TIMEDOUT) {
        logger.logMessage(LogLevel::WARNING, "[OpenDTU] Curl HTTP GET request timed out");
        return "failed";
    }

    if (res != CURLE_OK) {
        logger.logMessage(LogLevel::DEBUG, "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        return "failed";
    }

    // wait for the callback func to write response to the read buffer
    while(readBuffer == "") {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // give the callback function enough time to finish writing into the read buffer
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    return readBuffer;
}

// function for sending POST request with authentication
void OpenDtuInterface::sendPostRequest(const std::string &url, const std::string &jsonData) const
{
    CURLcode res;

    // set CURL options
    curl_easy_setopt(m_curl_post_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl_post_handle, CURLOPT_POSTFIELDS, jsonData.c_str());

    // perform the request
    res = curl_easy_perform(m_curl_post_handle);

    // check for errors
    if(res == CURLE_COULDNT_CONNECT) {
        logger.logMessage(LogLevel::ERROR, "[OpenDTU] Could not connect via HTTP");
        return;
    }

    if(res == CURLE_OPERATION_TIMEDOUT) {
        logger.logMessage(LogLevel::WARNING, "[OpenDTU] Curl HTTP POST request timed out");
        return;
    }

    if (res != CURLE_OK) {
        logger.logMessage(LogLevel::DEBUG, "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }
}