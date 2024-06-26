/*
	File: PsuController.cpp
	Edited by Elias Geiger
*/

#include "PsuController.h"

extern ConfigFile cfg;

// Constructor
PsuController::PsuController() {
	m_threadRunning = false;
	m_lastCurrentCmd = 0.0f;
	m_cmdAckFlag = false;
	m_secondsSinceLastCharge = 0;
}

// Destructor
PsuController::~PsuController() {}

// public methods // 
bool PsuController::setup(const char* interfaceName) {
	// initialize the slot detect control
	if(!initSlotDetect()) {
		std::cerr << "Failed to init slot detect control!" << std::endl;
		return false;
	}

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
		ptr->m_threadRunning = true;

		// Timing variables
		auto currentTime = steady_clock::now();
		auto lastStatusRequestTime = currentTime, lastCurrentCommandRepeatTime = currentTime;
		milliseconds timeElapsed;
		
		std::cout << "[PSU-thread] worker thread running ..." << std::endl;

		// send initial volatage and current commands, don't output power by default
		ptr->setMaxVoltage(cfg.getChargerAbsorptionVoltage(), false);		// online mode

		// send first request for status report
		ptr->requestStatusData();

		// repeat as long as CAN socket is valid
		while(ptr->m_threadRunning == true) {
			struct can_frame receivedCanFrame;

			// read in message from CAN bus
			int nbytes = read(ptr->m_canSocket, &receivedCanFrame, sizeof(can_frame));
			if (nbytes < 0) {
				std::cerr << "[PSU-thread] Problem with reading can message frame" << std::endl;
				continue;
			}

			// Detect message type
			switch (receivedCanFrame.can_id & 0x1FFFFFFF) {
				// status report message
				case 0x1081407F:
					ptr->updateLocalParams((uint8_t*)&receivedCanFrame.data);
					break;

				// ...
				case 0x1081D27F:
					// r4850_description((uint8_t *)&frame.data);
					// ...
					break;

				// command acknowledge message
				case 0x1081807E:
					// Acknowledgement //
					ptr->processAckFrame((uint8_t*)&receivedCanFrame.data);
					break;

				// unknown message type
				default:
					// printf("Unknown frame 0x%03X [%d] ", (recvFrame.can_id & 0x1FFFFFFF), recvFrame.can_dlc);
					break;
			}

			// every second request status update
			currentTime = steady_clock::now();
			timeElapsed = duration_cast<milliseconds>(currentTime - lastStatusRequestTime);
			if(timeElapsed.count() > 1000) {
				ptr->requestStatusData();
				lastStatusRequestTime = steady_clock::now();
			}

			// every 5 sec repeat last current command to ensure PSU stays in online mode
			currentTime = steady_clock::now();
			timeElapsed = duration_cast<milliseconds>(currentTime - lastCurrentCommandRepeatTime);
			if(timeElapsed.count() > 5000) {
				ptr->setMaxCurrent(ptr->m_lastCurrentCmd, false);
				if(ptr->m_lastCurrentCmd == 0.0f) {
					// tick the slot detect keep alive timer
					ptr->m_secondsSinceLastCharge += static_cast<unsigned int>(timeElapsed.count() / 1000);
					if(ptr->m_secondsSinceLastCharge >= cfg.getSlotDetectKeepAliveTime()) {
						// turn off slot detect to enter stand by mode for power saving
						#ifdef _TARGET_RASPI
							if(cfg.isSlotDetectControlEnabled()) {
								digitalWrite(SD_PIN, LOW);
								std::cout << "[PSU-thread] Turn off slot detect --> standby mode" << std::endl;
							}
						#endif
						ptr->m_secondsSinceLastCharge = 0;
					}
				} else {
					// reset slot detect keep alive timer when last current command greater than zero
					ptr->m_secondsSinceLastCharge = 0;
				}

				lastCurrentCommandRepeatTime = steady_clock::now();
			}
		}

		std::cout << "[PSU-thread] closeup --> finish thread now" << std::endl;
	}, this);

	return true;
}

void PsuController::shutdown() {
	// disable slot detect (on raspberry pi only)
	#ifdef _TARGET_RASPI 
		digitalWrite(SD_PIN, LOW);
		std::cout << "[PSU] Slot detect disabled before exit" << std::endl;
	#endif

	// wait for worker thread
	m_threadRunning = false;
	m_workerTh.join();

	// close the CAN socket
	if(close(m_canSocket) < 0) {
		std::cerr << "Could not close CAN socket! Not created at all?" << std::endl;
	}
}

void PsuController::printParams() const {
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
bool PsuController::setMaxVoltage(float voltage, bool nonvolatile) {
	struct can_frame dataFrameToSend;
	uint16_t value = voltage * 1024;		
	uint8_t volatilityFlag;

	// set volatility flag
	if (nonvolatile) volatilityFlag = 0x01;	// Off-line mode
	else		 volatilityFlag = 0x00;	// On-line mode

	// construct CAN message frame
	dataFrameToSend.can_id = 0x108180FE | CAN_EFF_FLAG;
	dataFrameToSend.can_dlc = 8;
	dataFrameToSend.data[0] = 0x01;
	dataFrameToSend.data[1] = volatilityFlag;
	dataFrameToSend.data[2] = 0x00;
	dataFrameToSend.data[3] = 0x00;
	dataFrameToSend.data[4] = 0x00;
	dataFrameToSend.data[5] = 0x00;
	dataFrameToSend.data[6] = (value & 0xFF00) >> 8;		// first the higher byte
	dataFrameToSend.data[7] = value & 0xFF;				// then the lower one

	// send the message frame
	if(!sendCanFrame(dataFrameToSend)) {
		std::cerr << "Failed to send voltage command!" << std::endl;
		return false;
	}
	return true;
}

// Does not block
bool PsuController::setMaxCurrent(float current, bool nonvolatile) {
	struct can_frame dataFrameToSend;
	uint16_t value = current * MAX_CURRENT_MULTIPLIER;
	uint8_t volatilityFlag;

	// set volatility flag
	if (nonvolatile) volatilityFlag = 0x04;					// Off-line mode
	else		 volatilityFlag = 0x03;						// On-line mode

	// construct CAN message frame
	dataFrameToSend.can_id = 0x108180FE | CAN_EFF_FLAG;
	dataFrameToSend.can_dlc = 8;
	dataFrameToSend.data[0] = 0x01;
	dataFrameToSend.data[1] = volatilityFlag;
	dataFrameToSend.data[2] = 0x00;
	dataFrameToSend.data[3] = 0x00;
	dataFrameToSend.data[4] = 0x00;
	dataFrameToSend.data[5] = 0x00;
	dataFrameToSend.data[6] = (value & 0xFF00) >> 8;		// first the higher bytes
	dataFrameToSend.data[7] = value & 0xFF;					// then the lower one

	// send the message frame
	if(!sendCanFrame(dataFrameToSend)) {
		std::cerr << "Failed to send current command!" << std::endl;
		return false;
	}

	// reset command acknowledgement flag if target current has changed
	if(current != m_lastCurrentCmd) {
		m_cmdAckFlag = false;
		std::cout << "[PSU] sent new current command: " << current << "A" << std::endl;

		// reenable slot detect after standby periods (on raspberry pi only)
		if(m_lastCurrentCmd == 0.0f && current > 0.0f) {
			#ifdef _TARGET_RASPI
				if(cfg.isSlotDetectControlEnabled()) {
					digitalWrite(SD_PIN, HIGH);
					std::cout << "[PSU] Slot detect (re)enabled" << std::endl;
				}
			#endif
		}
	}

	// save as last current command
	m_lastCurrentCmd = current;

	return true;
}

// Does not block
bool PsuController::requestStatusData() {
	struct can_frame requestFrame;

	// construct CAN message frame
	requestFrame.can_id = 0x108040FE | CAN_EFF_FLAG;
	// 0x108140FE also works 
	requestFrame.can_dlc = 8;
	requestFrame.data[0] = 0;
	requestFrame.data[1] = 0;
	requestFrame.data[2] = 0;
	requestFrame.data[3] = 0;
	requestFrame.data[4] = 0;
	requestFrame.data[5] = 0;
	requestFrame.data[6] = 0;
	requestFrame.data[7] = 0;

	// send the message frame
	if(!sendCanFrame(requestFrame)) {
		std::cerr << "Failed to send status request command!" << std::endl;
		return false;
	}

	return true;
}

float PsuController::getCurrentInputPower() const {
	return m_rectifierParams.input_power;
}

float PsuController::getCurrentOutputVoltage() const {
	return m_rectifierParams.output_voltage;
}

float PsuController::getCurrentOutputCurrent() const {
	return m_rectifierParams.output_current;
}

// generic helper method for sending out CAN frames
bool PsuController::sendCanFrame(struct can_frame frame) {

	// write out frame to the can bus
	if (write(m_canSocket, &frame, sizeof(can_frame)) != sizeof(can_frame)) {
		return false;
	}
	return true;
}

// processes a received status frame from the PSU
void PsuController::updateLocalParams(uint8_t *frame) {
	// decode and store value in payload of message frame
	uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

	// decode message frame and update local rectifier parameters accordingly
	switch (frame[1]) {

		// input related // 
		case R48xx_DATA_INPUT_POWER:
		{
			m_rectifierParams.input_power = value / 1024.0f;
			break;
		}
			
		case R48xx_DATA_INPUT_FREQ:
		{
			m_rectifierParams.input_frequency = value / 1024.0f;
			break;	
		}

		case R48xx_DATA_INPUT_VOLTAGE:
		{
			m_rectifierParams.input_voltage = value / 1024.0f;
			break;
		}

		case R48xx_DATA_INPUT_CURRENT:
		{
			m_rectifierParams.input_current = value / 1024.0f;
			break;
		}
			
		case R48xx_DATA_INPUT_TEMPERATURE:
		{
			m_rectifierParams.input_temp = value / 1024.0f;
			break;
		}
			
		// output related //
		case R48xx_DATA_OUTPUT_POWER:
		{
			m_rectifierParams.output_power = value / 1024.0f;
			break;
		}

		case R48xx_DATA_EFFICIENCY:
		{
			m_rectifierParams.efficiency = value / 1024.0f;
			break;
		}

		case R48xx_DATA_OUTPUT_VOLTAGE:
		{
			m_rectifierParams.output_voltage = value / 1024.0f;
			break;
		}

		case R48xx_DATA_OUTPUT_CURRENT1:		// --> alternative current measurement
		{
			// printf("Output Current(1) %.02fA\r\n", value / 1024.0);
			// rp->output_current = value / 1024.0;
			break;
		}

		case R48xx_DATA_OUTPUT_CURRENT:			// --> usually received at last
		{
			m_rectifierParams.output_current = value / 1024.0f;
			#ifdef _VERBOSE_OUTPUT
				this->printParams();
			#endif
			break;
		}

		case R48xx_DATA_OUTPUT_CURRENT_MAX:
		{
			m_rectifierParams.max_output_current = value / static_cast<float>(MAX_CURRENT_MULTIPLIER);
			break;
		}

		case R48xx_DATA_OUTPUT_TEMPERATURE:
		{
			m_rectifierParams.output_temp = value / 1024.0f;
			break;
		}

		default:
		{
			// printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
			break;
		}
			
	}
}

// process an acknowledge frame from the PSU
void PsuController::processAckFrame(uint8_t *frame) {
	// decode error flag and 
	bool error = frame[0] & 0x20;
	uint32_t value = __builtin_bswap32(*(uint32_t*)&frame[4]);

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
			float currentAck = static_cast<float>(value) / MAX_CURRENT_MULTIPLIER;
			if(m_cmdAckFlag == false && currentAck == m_lastCurrentCmd) {
				printf("%s setting online current to %.02fA\n", error ? "Error" : "Success", currentAck);
				m_cmdAckFlag = true;
			}
			break;
		}
			
		case 0x04:
		{
			printf("%s setting non-volatile (offline) current to %.02fA\n", error ? "Error" : "Success", static_cast<float>(value) / MAX_CURRENT_MULTIPLIER);
			break;
		}
			
		default:
		{
			printf("%s setting unknown parameter (0x%02X)\n", error ? "Error" : "Success", frame[1]);
		}
	}
}

// setup wiringpi for direct GPIO interfacing (on raspberry pi only)
bool PsuController::initSlotDetect() {
	#ifdef _TARGET_RASPI
		wiringPiSetupGpio();
		pinMode(SD_PIN, OUTPUT);

		// turn off slot detect at application startup if feature is enabled
		if(cfg.isSlotDetectControlEnabled()) {
			digitalWrite(SD_PIN, LOW);
		} else {
			digitalWrite(SD_PIN, HIGH);		// when sd control disabled just turn on once 
		}
		std::cout << "[PSU] Slot detect initialized" << std::endl;
	#endif

	return true;
}