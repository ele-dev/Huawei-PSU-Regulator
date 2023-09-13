/*
	File: PsuController.cpp
	This class handles CAN communication with the Huawei R4850G2 power supply

	Code from original repository: https://github.com/craigpeacock/Huawei_R4850G2_CAN
	Edited by Elias Geiger
*/

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

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

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

extern ConfigFile cfg;

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

public:
	PsuController() {
		std::cout << "[PSU] Controller created" << std::endl;
		m_threadRunning = false;
		m_lastCurrentCmd = 0.0f;
		m_cmdAckFlag = false;
	}

	// Destructor
	~PsuController() {
		std::cout << "[PSU] controller destructed" << std::endl;
	}

	// public methods
	bool setup(const char* interfaceName) {
		// create can socket
		m_canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
		if(m_canSocket < 0) {
			std::cerr << "Failed to create CAN socket!" << std::endl;
			return false;
		}

		// use the specified interface
		strcpy(m_ifr.ifr_name, interfaceName);
		ioctl(m_canSocket, SIOCGIFINDEX, &m_ifr);

		// prepare CAN adress
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.can_family = AF_CAN;
		m_addr.can_ifindex = m_ifr.ifr_ifindex;

		// bind address to interface
		if(bind(m_canSocket, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
			std::cerr << "Failed to bind CAN Socket!" << std::endl;
			return false;
		}

		// spawn worker thread
		m_workerTh = std::thread([] (PsuController* ptr) {
			auto lastTime = steady_clock::now(), lastTime2 = steady_clock::now();
			auto currentTime = steady_clock::now(), currentTime2 = steady_clock::now();
			ptr->m_threadRunning = true;
			std::cout << "[PSU-thread] worker thread running ..." << std::endl;

			// send initial volatage and current commands, don't output power by default
			ptr->setMaxVoltage(cfg.getChargerAbsorptionVoltage(), false);		// online mode
			// ptr->setMaxCurrent(0.0f, false);		// " "

			// send first request for status report
			ptr->requestStatusData();

			// repeat as long as CAN socket is valid
			while(ptr->m_threadRunning == true) {
				struct can_frame recvFrame;

				// read in message from CAN bus
				int nbytes = read(ptr->m_canSocket, &recvFrame, sizeof(can_frame));
				if (nbytes < 0) {
					std::cerr << "[PSU-thread] Problem with reading can message frame" << std::endl;
					continue;
				}

				// Detect message type
				switch (recvFrame.can_id & 0x1FFFFFFF) {
					// status report message
					case 0x1081407F:
						// r4850_data((uint8_t *)&frame.data, &rp);
						ptr->updateParams((uint8_t*)&recvFrame.data);
						break;

					// ...
					case 0x1081D27F:
						// r4850_description((uint8_t *)&frame.data);
						// ...
						break;

					// command acknowledge message
					case 0x1081807E:
						// Acknowledgement //
						// r4850_ack((uint8_t *)&frame.data);
						ptr->processAckFrame((uint8_t*)&recvFrame.data);
						break;

					// unknown message type
					default:
						// printf("Unknown frame 0x%03X [%d] ", (recvFrame.can_id & 0x1FFFFFFF), recvFrame.can_dlc);
						break;
				}

				// every second request status update
				currentTime = steady_clock::now();
				milliseconds timeElapsed = duration_cast<milliseconds>(currentTime - lastTime);
				if(timeElapsed.count() > 1000) {
					ptr->requestStatusData();
					lastTime = steady_clock::now();
				}

				// every 5 sec repeat last current command to ensure PSU stays in online mode
				currentTime2 = steady_clock::now();
				timeElapsed = duration_cast<milliseconds>(currentTime2 - lastTime2);
				if(timeElapsed.count() > 5000) {
					ptr->setMaxCurrent(ptr->m_lastCurrentCmd, false);
					lastTime2 = steady_clock::now();
				}
			}

			std::cout << "[PSU-thread] closeup --> finish thread now" << std::endl;
		}, this);

		return true;
	}

	void shutdown() {
		std::cout << "try stop thread from running" << std::endl;
		m_threadRunning = false;

		// wait for worker thread
		m_workerTh.join();

		// close the CAN socket
		if(close(m_canSocket) < 0) {
			std::cerr << "Could not close CAN socket! Not created at all?" << std::endl;
		}
	}

	void printParams() {
		// printf("\033[2J\n");
		std::cout << std::endl;
		printf("Input Voltage %.02fV @ %.02fHz\n", m_rectifierParams.input_voltage, m_rectifierParams.input_frequency);
		printf("Input Current %.02fA\n", m_rectifierParams.input_current);
		printf("Input Power %.02fW\n", m_rectifierParams.input_power);

		std::cout << std::endl;
		printf("Output Voltage %.02fV\n", m_rectifierParams.output_voltage);
		printf("Output Current %.02fA\n", m_rectifierParams.output_current);
		printf("Output Power %.02fW\n", m_rectifierParams.output_power);

		std::cout << std::endl;
		printf("Input Temperature %.01f DegC\n", m_rectifierParams.input_temp);
		printf("Output Temperature %.01f DegC\n", m_rectifierParams.output_temp);
		printf("Efficiency %.01f%%\n", m_rectifierParams.efficiency * 100);
	}

	// Does not block
	bool setMaxVoltage(float voltage, bool nonvolatile) {
		struct can_frame sendFrame;
		uint16_t value = voltage * 1024;
		uint8_t command;

		if (nonvolatile) command = 0x01;	// Off-line mode
		else		 command = 0x00;	// On-line mode

		// construct CAN message frame
		sendFrame.can_id = 0x108180FE | CAN_EFF_FLAG;
		sendFrame.can_dlc = 8;
		sendFrame.data[0] = 0x01;
		sendFrame.data[1] = command;
		sendFrame.data[2] = 0x00;
		sendFrame.data[3] = 0x00;
		sendFrame.data[4] = 0x00;
		sendFrame.data[5] = 0x00;
		sendFrame.data[6] = (value & 0xFF00) >> 8;
		sendFrame.data[7] = value & 0xFF;

		if(!sendCanFrame(sendFrame)) {
			std::cerr << "Failed to send voltage command!" << std::endl;
			return false;
		}
		return true;
	}

	// Does not block
	bool setMaxCurrent(float current, bool nonvolatile) {
		struct can_frame sendFrame;
		uint16_t value = current * MAX_CURRENT_MULTIPLIER;
		uint8_t command;

		if (nonvolatile) command = 0x04;	// Off-line mode
		else		 command = 0x03;	// On-line mode

		// construct CAN message frame
		sendFrame.can_id = 0x108180FE | CAN_EFF_FLAG;
		sendFrame.can_dlc = 8;
		sendFrame.data[0] = 0x01;
		sendFrame.data[1] = command;
		sendFrame.data[2] = 0x00;
		sendFrame.data[3] = 0x00;
		sendFrame.data[4] = 0x00;
		sendFrame.data[5] = 0x00;
		sendFrame.data[6] = (value & 0xFF00) >> 8;
		sendFrame.data[7] = value & 0xFF;

		if(!sendCanFrame(sendFrame)) {
			std::cerr << "Failed to send current command!" << std::endl;
			return false;
		}

		// reset command acknowledgement flag if target current has changed
		if(current != m_lastCurrentCmd) {
			m_cmdAckFlag = false;
			std::cout << "[PSU] sent new current command: " << current << "A" << std::endl;
		}

		// save as last current command
		m_lastCurrentCmd = current;

		return true;
	}

	// Does not block
	bool requestStatusData() {
		struct can_frame sendFrame;

		// construct CAN message frame
		sendFrame.can_id = 0x108040FE | CAN_EFF_FLAG;
		// 0x108140FE also works 
		sendFrame.can_dlc = 8;
		sendFrame.data[0] = 0;
		sendFrame.data[1] = 0;
		sendFrame.data[2] = 0;
		sendFrame.data[3] = 0;
		sendFrame.data[4] = 0;
		sendFrame.data[5] = 0;
		sendFrame.data[6] = 0;
		sendFrame.data[7] = 0;

		if(!sendCanFrame(sendFrame)) {
			std::cerr << "Failed to send status request command!" << std::endl;
			return false;
		}

		return true;
	}

	float getCurrentIntputPower() const {
		return m_rectifierParams.input_power;
	}

	float getCurrentOutputVoltage() const {
		return m_rectifierParams.output_voltage;
	}

	float getCurrentOutputCurrent() const {
		return m_rectifierParams.output_current;
	}

private:
	// generic helper method for sending out CAN frames
	// Does not block
	bool sendCanFrame(struct can_frame frame) {

		// write out frame to the can bus
		if (write(m_canSocket, &frame, sizeof(can_frame)) != sizeof(can_frame)) {
			return false;
		}
		return true;
	}

	// processes a received status frame from the PSU
	void updateParams(uint8_t *frame) {
		// decode and store value in payload of message frame
		uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

		// decode message frame and update parameter accordingly
		switch (frame[1]) {

			// input related // 
			case R48xx_DATA_INPUT_POWER:
				m_rectifierParams.input_power = value / 1024.0;
				break;

			case R48xx_DATA_INPUT_FREQ:
				m_rectifierParams.input_frequency = value / 1024.0;
				break;

			case R48xx_DATA_INPUT_VOLTAGE:
				m_rectifierParams.input_voltage = value / 1024.0;
				break;

			case R48xx_DATA_INPUT_CURRENT:
				m_rectifierParams.input_current = value / 1024.0;
				break;

			case R48xx_DATA_INPUT_TEMPERATURE:
				m_rectifierParams.input_temp = value / 1024.0;
				break;
			
			// output related //
			case R48xx_DATA_OUTPUT_POWER:
				m_rectifierParams.output_power = value / 1024.0;
				break;

			case R48xx_DATA_EFFICIENCY:
				m_rectifierParams.efficiency = value / 1024.0;
				break;

			case R48xx_DATA_OUTPUT_VOLTAGE:
				m_rectifierParams.output_voltage = value / 1024.0;
				break;

			case R48xx_DATA_OUTPUT_CURRENT1:		// --> alternative current measurement
				// printf("Output Current(1) %.02fA\r\n", value / 1024.0);
				// rp->output_current = value / 1024.0;
				break;

			case R48xx_DATA_OUTPUT_CURRENT:			// --> usually received at last
				m_rectifierParams.output_current = value / 1024.0;
				#ifdef _VERBOSE_OUTPUT
					this->printParams();
				#endif
				break;

			case R48xx_DATA_OUTPUT_CURRENT_MAX:
				m_rectifierParams.max_output_current = value / MAX_CURRENT_MULTIPLIER;
				break;

			case R48xx_DATA_OUTPUT_TEMPERATURE:
				m_rectifierParams.output_temp = value / 1024.0;
				break;

			default:
				// printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
				break;
		}
	}

	// process a acknowledge frame from the PSU
	void processAckFrame(uint8_t *frame) {
		// decode error flag and 
		bool error = frame[0] & 0x20;
		uint32_t value = __builtin_bswap32(*(uint32_t*)&frame[4]);
		float currentAck = 0.0f;

		switch (frame[1]) {
			case 0x00:
			{
				printf("%s setting online voltage to %.02fV\n", error ? "Error" : "Success", value / 1024.0);
				break;
			}
				
			case 0x01:
			{
				printf("%s setting non-volatile (offline) voltage to %.02fV\n", error ? "Error" : "Success", value / 1024.0);
				break;
			}
				
			case 0x02:
			{
				printf("%s setting overvoltage protection to %.02fV\n", error ? "Error" : "Success", value / 1024.0);
				break;
			}
				
			case 0x03:
			{
				currentAck = (float)value / MAX_CURRENT_MULTIPLIER;
				if(m_cmdAckFlag == false && currentAck == m_lastCurrentCmd) {
					printf("%s setting online current to %.02fA\n", error ? "Error" : "Success", currentAck);
					m_cmdAckFlag = true;
				}
				break;
			}
				
			case 0x04:
			{
				printf("%s setting non-volatile (offline) current to %.02fA\n", error ? "Error" : "Success", (float)value / MAX_CURRENT_MULTIPLIER);
				break;
			}
				
			default:
			{
				printf("%s setting unknown parameter (0x%02X)\n", error ? "Error" : "Success", frame[1]);
			}
		}
	}
};
