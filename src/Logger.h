/*
    File: Logger.h
    This module provides a central thread-safe logger functionality

    written by Elias Geiger
*/

#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <mutex>

// define the available log levels (ordered by severity)
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger 
{
    std::ofstream m_logFile;
    std::mutex m_logMutex;

public:
    Logger();
    ~Logger();

    void logMessage(LogLevel level, const std::string& message);

private:
    std::string getCurrentTimestamp() const;
};