/*
    File: Logger.cpp
    written by Elias Geiger
*/

#include "Logger.h"

Logger::Logger() {
}

Logger::~Logger() {
}

void Logger::logMessage(LogLevel level, const std::string& message) {
    // lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(this->m_logMutex);

    // Get the current timestamp
    std::string timestamp = getCurrentTimestamp();

    // Convert log level enum to string
    std::string logLevelStr;
    switch (level) {
        case LogLevel::DEBUG:
            logLevelStr = "DEBUG";
            break;
        case LogLevel::INFO:
            logLevelStr = "INFO";
            break;
        case LogLevel::WARNING:
            logLevelStr = "WARNING";
            break;
        case LogLevel::ERROR:
            logLevelStr = "ERROR";
            break;
        default:
            logLevelStr = "UNKNOWN";
            break;
    }

    // Format and print the log message
    std::cout << timestamp << " - " << logLevelStr << " - " << message << std::endl;

    // write the same message to the logfile if needed
    // ...
}

std::string Logger::getCurrentTimestamp() const {
    using namespace std::chrono;

    // Get current time as time_point
    auto now = system_clock::now();

    // Convert to time_t for formatting the date part
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    // Get the milliseconds part
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // Format the timestamp
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << ','  << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}