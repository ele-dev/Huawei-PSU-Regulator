/*
    File: Utils.cpp

    written by Elias Geiger
*/

#include "Utils.h"

// helper function for detecting a scheduled exit event to close the application
bool scheduledClose() {
    if(cfg.isScheduledExitEnabled()) {
        // get current system time
        time_t currTime = time(NULL);
        tm* tm_local = localtime(&currTime);
        
        // check if time for scheduled exit has passed
        if(tm_local->tm_hour >= cfg.getScheduledExitHour() && tm_local->tm_min >= cfg.getScheduledExitMinute()) {
            return true;
        }
    }

    return false;
}

// function for sending HTTP GET requests with authentication
std::string sendGetRequest(const std::string &url, const std::string &user, const std::string &password)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer = "";

    // Initialize CURL
    curl = curl_easy_init();
    if (curl)
    {
        // Concatenate user and password for the credentials
        std::string credentials = user + ":" + password;

        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // Print the response
            // std::cout << "Response: " << readBuffer << std::endl;
        }

        // Cleanup
        curl_easy_cleanup(curl);

        return readBuffer;
    }
    else
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    return "";
}

// function for sending HTTP POST requests with authentication
void sendPostRequest(const std::string &url, const std::string &user, const std::string &password, const std::string &jsonData)
{
    CURL *curl;
    CURLcode res;

    // Initialize CURL
    curl = curl_easy_init();
    if (curl)
    {
        // Concatenate user and password for the credentials
        std::string credentials = user + ":" + password;

        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }
}

