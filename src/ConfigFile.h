/*
    File: ConfigFile.h

    written by Elias Geiger
*/

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

#include "config.h"

class ConfigFile
{
    std::string m_fileName;

    // config variables
    std::string m_canInterfaceName;
    short m_udpListenerPort;
    short m_minChargePower, m_maxChargePower, m_targetGridPower;
    int m_regulatorIdleTime,  m_regulatorErrorThreshold;
    float m_chargerAbsorptionVoltage;
    bool m_scheduledExitEnabled;
    int m_scheduledExitHour, m_scheduledExitMinute;

public:
    ConfigFile(std::string);
    ~ConfigFile();

    bool loadConfig();
    void printConfig() const;

    // Getters // 
    const char* getCanInterfaceName() const;
    short getUdpPort() const;
    short getMinChargePower() const;
    short getMaxChargePower() const;
    short getTargetGridPower() const;
    int getRegulatorErrorThreshold() const;
    int getRegulatorIdleTime() const;
    float getChargerAbsorptionVoltage() const;
    bool isScheduledExitEnabled() const;
    int getScheduledExitHour() const;
    int getScheduledExitMinute() const;

private:
    void parseLine(std::string);
    std::vector<std::string> split(const std::string&, char);

};