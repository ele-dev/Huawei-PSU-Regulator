/*
    File: PsuController.h
    This class handles CAN communication with the Huawei R4850G2 power supply

	Code from original repository: https://github.com/craigpeacock/Huawei_R4850G2_CAN
	Edited by Elias Geiger
*/

#pragma once

// Includes
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <signal.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "ConfigFile.h"

#ifdef _TARGET_RASPI
	#include <wiringPi.h>
#endif

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

// GPIO pin that controls the slot detect relay (active high)
#define SD_PIN 17

// config variables ---------------------
#define MAX_CURRENT_MULTIPLIER		20
#define R48xx_DATA_INPUT_POWER		0x70
#define R48xx_DATA_INPUT_FREQ		0x71
#define R48xx_DATA_INPUT_CURRENT	0x72
#define R48xx_DATA_OUTPUT_POWER		0x73
#define R48xx_DATA_EFFICIENCY		0x74
#define R48xx_DATA_OUTPUT_VOLTAGE	0x75
#define R48xx_DATA_OUTPUT_CURRENT_MAX	0x76
#define R48xx_DATA_INPUT_VOLTAGE	0x78
#define R48xx_DATA_OUTPUT_TEMPERATURE	0x7F
#define R48xx_DATA_INPUT_TEMPERATURE	0x80
#define R48xx_DATA_OUTPUT_CURRENT	0x81
#define R48xx_DATA_OUTPUT_CURRENT1	0x82
// ---------------------------------------

// struct represents a state including all parameters of the PSU
struct RectifierParameters
{
	float input_voltage;
	float input_frequency;
	float input_current;
	float input_power;
	float input_temp;
	float efficiency;
	float output_voltage;
	float output_current;
	float max_output_current;
	float output_power;
	float output_temp;
	float amp_hour;
};

class PsuController 
{
    // stores latest state of the PSU params
	struct RectifierParameters m_rectifierParams;

	// CAN related 
	struct sockaddr_can m_addr;
	int m_canSocket;
	struct ifreq m_ifr;

	std::thread m_workerTh;
	std::atomic<bool> m_threadRunning;
	float m_lastCurrentCmd;
	bool m_cmdAckFlag;
	unsigned int m_secondsSinceLastCharge;

public:
    PsuController();
    ~PsuController();

    bool setup(const char*);
    void shutdown();
    void printParams();  
    bool setMaxVoltage(float, bool);
    bool setMaxCurrent(float, bool);
    bool requestStatusData();

    // getters //
    float getCurrentInputPower() const;
    float getCurrentOutputVoltage() const;
    float getCurrentOutputCurrent() const;

private:
    // helper methods //
    bool sendCanFrame(struct can_frame);
    void updateParams(uint8_t*);
    void processAckFrame(uint8_t*);
	bool initSlotDetect();
};