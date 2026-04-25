/*
    File: ConfigFile.hpp
    written by Elias Geiger
*/

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

#include "Utils.hpp"
#include "default-conf.hpp"

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

    bool LoadConfig();
    void PrintConfig() const;

    // Getters // 
    const char* GetCanInterfaceName() const;
    short GetUdpPort() const;
    short GetMinChargePower() const;
    short GetMaxChargePower() const;
    short GetTargetGridPower() const;
    int GetRegulatorErrorThreshold() const;
    int GetRegulatorIdleTime() const;
    float GetChargerAbsorptionVoltage() const;
    bool IsScheduledExitEnabled() const;
    int GetScheduledExitHour() const;
    int GetScheduledExitMinute() const;
    bool IsSlotDetectControlEnabled() const;
    int GetSlotDetectKeepAliveTime() const;

    std::string GetOpenDtuHost() const;
    std::string GetOpenDtuAdminUser() const;
    std::string GetOpenDtuAdminPassword() const;
    std::string GetOpenDtuBatteryInverterId() const;
    float GetOpenDtuStartDischargeVoltage() const;
    float GetOpenDtuStopDischargeVoltage() const;

    const char* GetPowerMeterModbusIp() const;
    short GetPowerMeterModbusPort() const;
    int GetPowerMeterModbusPollingPeriod() const;

private:
    void ParseLine(std::string);
    std::vector<std::string> Split(const std::string&, char);

};
