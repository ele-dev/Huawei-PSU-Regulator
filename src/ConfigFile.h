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

#include "Utils.h"
#include "default-conf.h"

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
    bool m_slotDetectCtlEnabled;
    int m_slotDetectKeepAliveTime;

    std::string m_openDtuHost;
    std::string m_openDtuAdminUser;
    std::string m_openDtuAdminPass;
    std::string m_openDtuBatteryInvId;
    float m_openDtuStartDischargeVoltage;
    float m_openDtuStopDischargeVoltage;

    std::string m_powerMeterModbusIp;
    short m_powerMeterModbusPort;
    int m_powerMeterModbusPollingPeriod;

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
    bool isSlotDetectControlEnabled() const;
    int getSlotDetectKeepAliveTime() const;

    std::string getOpenDtuHost() const;
    std::string getOpenDtuAdminUser() const;
    std::string getOpenDtuAdminPassword() const;
    std::string getOpenDtuBatteryInverterId() const;
    float getOpenDtuStartDischargeVoltage() const;
    float getOpenDtuStopDischargeVoltage() const;

    const char* getPowerMeterModbusIp() const;
    short getPowerMeterModbusPort() const;
    int getPowerMeterModbusPollingPeriod() const;

private:
    void parseLine(std::string);
    std::vector<std::string> split(const std::string&, char);

};