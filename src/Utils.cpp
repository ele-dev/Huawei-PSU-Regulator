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

// Helper function to round float values on decimals
float round(float var)
{
    float value = (int)(var * 100 + .5);
    return static_cast<float>(value) / 100;
}