/*
    File: Utils.cpp
    written by Elias Geiger
*/

#include "Utils.hpp"

extern ConfigFile cfg;

// helper function for detecting a scheduled exit event to close the application
bool ScheduledClose() {
    if(cfg.IsScheduledExitEnabled()) {
        // get current system time
        time_t currTime = time(NULL);
        tm* tm_local = localtime(&currTime);
        
        // check if time for scheduled exit has passed
        if(tm_local->tm_hour >= cfg.GetScheduledExitHour() && tm_local->tm_min >= cfg.GetScheduledExitMinute()) {
            return true;
        }
    }

    return false;
}

// Helper function to round float values on decimals
float Round(float value)
{
    float val = (int)(value * 100 + .5);
    return static_cast<float>(val) / 100;
}

// Helper function to quickly get string representation of a float value
std::string Float2String(float value, int decimalDigits) 
{
    std::stringstream ss;
    ss.precision(decimalDigits);        // Set precision to 2 decimal places
    ss << std::fixed << value;
    std::string str = ss.str();
    return str;
}
