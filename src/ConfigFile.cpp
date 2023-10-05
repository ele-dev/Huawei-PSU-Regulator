/*
    File: ConfigFile.cpp

    written by Elias Geiger
*/

#include "ConfigFile.h"

ConfigFile::ConfigFile(std::string filename) {
    m_fileName = filename;
    
    // set config variable to default values
    m_udpListenerPort = UDP_PORT;
    m_canInterfaceName = CAN_INTERFACE_NAME;
    m_minChargePower = MIN_CHARGE_POWER;
    m_maxChargePower = MAX_CHARGE_POWER;
    m_targetGridPower = TARGET_GRID_POWER;
    m_regulatorErrorThreshold = REGULATOR_ERR_THRESHOLD;
    m_regulatorIdleTime = REGULATOR_IDLE_TIME;
    m_chargerAbsorptionVoltage = CHARGER_ABSORPTION_VOLTAGE;
    m_scheduledExitEnabled = SCHEDULED_EXIT_ENABLED;
    m_scheduledExitHour = SCHEDULED_EXIT_HOUR;
    m_scheduledExitMinute = SCHEDULED_EXIT_MINUTE;
    m_slotDetectCtlEnabled = SD_CONTROL_ENABLED;
    m_slotDetectKeepAliveTime = SD_KEEP_ALIVE_TIME;
}

ConfigFile::~ConfigFile() {}

// tries to open file. returns false on failure
bool ConfigFile::loadConfig() {
    // attempt to open config file from filesystem
    std::ifstream fileIn(m_fileName.c_str(), std::ifstream::in);
    if(!fileIn.is_open()) {
        return false;
    }

    // read in line by line
    std::string line = "";
    while(std::getline(fileIn, line)) {
        // skip empty lines and comment lines
        if(line.length() < 1) 
            continue;

        if(line[0] == '#')
            continue;

        // parse line 
        parseLine(line);
    }

    // close file stream
    fileIn.close();

    return true;
}

// method for printing all config variables to the console
void ConfigFile::printConfig() const {
    std::cout << "\nConfig Variables:" << std::endl;
    std::cout << "UDP Listener Port:          " << m_udpListenerPort << std::endl;
    std::cout << "CAN interface:              " << m_canInterfaceName << std::endl;
    std::cout << "Target grid power:          " << m_targetGridPower << " W" << std::endl;
    std::cout << "Min charge power:           " << m_minChargePower << " W" << std::endl;
    std::cout << "Max charge power:           " << m_maxChargePower << " W" << std::endl;
    std::cout << "Regulator error threshold:  " << m_regulatorErrorThreshold << " W" << std::endl;
    std::cout << "Regulator idle time:        " << m_regulatorIdleTime << " msec" << std::endl;
    std::cout << "Charger absorption voltage: " << m_chargerAbsorptionVoltage << " V" << std::endl;
    std::cout << "Scheduled exit enabled:     " << (m_scheduledExitEnabled ? "yes" : "no") << std::endl;
    std::cout << "Scheduled exit time:        " << m_scheduledExitHour << ":" << m_scheduledExitMinute << std::endl;
    std::cout << "Slot detect control:        " << (m_slotDetectCtlEnabled ? "active" : "not active") << std::endl;
    std::cout << "Slot detect keep alive:     " << m_slotDetectKeepAliveTime << " sec" << std::endl;
    std::cout << std::endl;
}

// method for parsing lines of the config file
void ConfigFile::parseLine(std::string line) {
    // remove remaining whitespaces from the line
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

    // split string into key value pair with : as delimiter
    std::vector<std::string> pair = split(line, ':');

    if(pair.size() != 2) {
        std::cerr << "[Config] Invalid line in config file!" << std::endl;
        return;
    }

    // detect config variable and store it
    try {
        std::string key = pair.at(0), value = pair.at(1);
        if(key == "can-interface") {
            m_canInterfaceName = value;
        } else if(key == "min-charge-power") {
            m_minChargePower = static_cast<short>(stoi(value));
        } else if(key == "max-charge-power") {
            m_maxChargePower = static_cast<short>(stoi(value));
        } else if(key == "target-grid-power") {
            m_targetGridPower = static_cast<short>(stoi(value));
        } else if(key == "regulator-error-threshold") {
            m_regulatorErrorThreshold = stoi(value);
        } else if(key == "udp-listener-port") {
            m_udpListenerPort = static_cast<short>(stoi(value));
        } else if(key == "regulator-idle-time") {
            m_regulatorIdleTime = stoi(value);
        } else if(key == "absorption-voltage") {
            m_chargerAbsorptionVoltage = stof(value);
        } else if(key == "scheduled-exit-enabled") {
            m_scheduledExitEnabled = value == "true" ? true : false;
        } else if(key == "scheduled-exit-hour") {
            m_scheduledExitHour = stoi(value);
            if(m_scheduledExitHour < 0) {
                std::cerr << "fix your entries for scheduled exit time in the config.txt file!" << std::endl;
                m_scheduledExitHour = 0;
            }
            if(m_scheduledExitHour > 23) {
                std::cerr << "fix your entries for scheduled exit time in the config.txt file!" << std::endl;
                m_scheduledExitHour = 23;
            }
        } else if(key == "scheduled-exit-minute") {
            m_scheduledExitMinute = stoi(value);
            if(m_scheduledExitMinute < 0) {
                std::cerr << "fix your entries for scheduled exit time in the config.txt file!" << std::endl;
                m_scheduledExitMinute = 0;
            }
            if(m_scheduledExitMinute > 59) {
                std::cerr << "fix your entries for scheduled exit time in the config.txt file!" << std::endl;
                m_scheduledExitMinute = 59;
            }
        } else if(key == "slotdetect-control-enabled") {
            m_slotDetectCtlEnabled = value == "true" ? true : false;
        } else if(key == "slotdetect-keep-alive-time") {
            m_slotDetectKeepAliveTime = stoi(value);
            // must be at least 10 seconds long
            if(m_slotDetectKeepAliveTime < 10) {
                std::cerr << "slot detect keep alive time must be at least 10 seconds!" << std::endl;
                m_slotDetectKeepAliveTime = SD_KEEP_ALIVE_TIME;
            }
        } else {
            std::cerr << "[Config] Invalid config variable named " << key << std::endl;
        }
    } catch(...) {
        std::cerr << "Exception catched while parsing config file!" << std::endl;
    }
}

// helper function for a basic string split operation
std::vector<std::string> ConfigFile::split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}

// Getters //
const char* ConfigFile::getCanInterfaceName() const {
    return m_canInterfaceName.c_str();
}

short ConfigFile::getUdpPort() const {
    return m_udpListenerPort;
}

short ConfigFile::getMinChargePower() const {
    return m_minChargePower;
}

short ConfigFile::getMaxChargePower() const {
    return m_maxChargePower;
}

short ConfigFile::getTargetGridPower() const {
    return m_targetGridPower;
}

int ConfigFile::getRegulatorErrorThreshold() const {
    return m_regulatorErrorThreshold;
}

int ConfigFile::getRegulatorIdleTime() const {
    return m_regulatorIdleTime;
}

float ConfigFile::getChargerAbsorptionVoltage() const {
    return m_chargerAbsorptionVoltage;
}

bool ConfigFile::isScheduledExitEnabled() const {
    return m_scheduledExitEnabled;
}

int ConfigFile::getScheduledExitHour() const {
    return m_scheduledExitHour;
}

int ConfigFile::getScheduledExitMinute() const {
    return m_scheduledExitMinute;
}

bool ConfigFile::isSlotDetectControlEnabled() const {
    return m_slotDetectCtlEnabled;
}

int ConfigFile::getSlotDetectKeepAliveTime() const {
    return m_slotDetectKeepAliveTime;
}