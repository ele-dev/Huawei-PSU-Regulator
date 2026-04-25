/*
    File: Logger.cpp
    written by Elias Geiger
*/

#include "Logger.hpp"

Logger::Logger(const char* logfilename) {
    this->m_logFile.open(logfilename, std::ios_base::app);
    this->LogMessage(LogChannel::INFO, "-------------------- APPLICATION LAUNCH --------------------");
}

Logger::~Logger() {
    this->m_logFile.close();
}

void Logger::LogMessage(LogChannel level, const std::string& message) {
    // lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(this->m_logMutex);

    // Get the current timestamp
    std::string timestamp = GetCurrentTimestamp();

    // Convert log level enum to string
    std::string logLevelStr;
    switch (level) {
        case LogChannel::DEBUG:
            logLevelStr = "DEBUG";
            break;
        case LogChannel::INFO:
            logLevelStr = "INFO";
            break;
        case LogChannel::WARNING:
            logLevelStr = "WARNING";
            break;
        case LogChannel::ERROR:
            logLevelStr = "ERROR";
            break;
        default:
            logLevelStr = "UNKNOWN";
            break;
    }

    // format and print the log message
    std::ostringstream log_entry;
    log_entry << timestamp << " - " << logLevelStr << " - " << message;

    // print to the console
    std::cout << log_entry.str() << std::endl;

    // write the same message to the logfile if needed
    if(this->m_logFile.is_open()) {
        this->m_logFile << log_entry.str() << std::endl;
    } else {
        std::cerr << "Error: Log file could not be opened!" << std::endl;
    }
}

std::string Logger::GetCurrentTimestamp() const {
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
