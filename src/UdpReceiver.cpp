/*
    File: UdpReceiver.cpp
    written by Elias Geiger
*/

#include "UdpReceiver.h"

extern Queue<short> cmdQueue;

UdpReceiver::UdpReceiver() {
    m_threadRunning = false;
    std::cout << "[UDP] receiver constructed" << std::endl;
}

UdpReceiver::~UdpReceiver() {
    std::cout << "[UDP] receiver destructed" << std::endl;
}

bool UdpReceiver::setup(short udpPort) {
    // only setup once
    if(m_threadRunning) {
        return false;
    }

    // create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(m_socket < 0) {
        std::cerr << "failed to create udp socket!" << std::endl;
        return false;
    }

    // create server address
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(static_cast<uint16_t>(udpPort));

    // bind socket to address and port 
    if(bind(m_socket, (const struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        std::cerr << "Failed to bind UDP socket!" << std::endl;
        return false;
    }

    // make socket non-blocking
    unsigned long setting = 1;
    if(ioctl(m_socket, FIONBIO, &setting) < 0) {
        std::cerr << "Failed to set non-blocking IO mode" << std::endl;
        return false;
    }

    // launch listener thread 
    m_listenerThread = std::thread([](UdpReceiver* ptr) {
        ptr->m_threadRunning = true;
        std::cout << "[UDP-thread] listener thread running ..." << std::endl;
    
        // construct client address
        struct sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));

        socklen_t len;
        len = sizeof(clientAddr);

        auto lastMsgTime = steady_clock::now();
		auto currentTime = steady_clock::now();
        const short fakePowerCmd = 30000;   

        // read and queue incoming messages until external stop signal
        char recvBuffer[MSGLEN];
        int bytesRead = 0;
        while(ptr->m_threadRunning) {
            bytesRead = recvfrom(ptr->m_socket, (char*)recvBuffer, MSGLEN, 0, (sockaddr*) &clientAddr, &len);
            if(bytesRead <= 0) {
                // check the time passed since the last message was received
                currentTime = steady_clock::now();
                seconds timeElapsed = duration_cast<seconds>(currentTime - lastMsgTime);
                if(timeElapsed.count() > 60) {
                    // Tasmota smart meter downtime detected --> send high power command to set 0W charge power
                    std::cerr << "[UDP-thread] Tasmota energy meter downtime detected! (timeout after 60s)" << std::endl;
                    cmdQueue.push(fakePowerCmd);
                    sleep_for(seconds(5));
                }
                continue;
            }
            recvBuffer[bytesRead] = '\0';     // String nulltermination

            // string to short conversion 
            short powerVal = static_cast<short>(atoi(recvBuffer));
            
            // filter out invalid unrealistic value (likely corrupted during transmission)
            if(powerVal < -30000 || powerVal > 20000) {
                std::cerr << "[UDP-thread] Received invalid power state value: " << powerVal << " (ignore)" << std::endl;
                continue;
            } else {
                std::cout << "[UDP-thread] Received updated power state: " << powerVal << std::endl;
            }

            // Put new value on the command queue for processing
            cmdQueue.push(powerVal);

            // save timestamp of last received message for downtime detection 
            lastMsgTime = steady_clock::now();
        }
                
        std::cout << "[UDP-thread] closeup --> finish thread now" << std::endl;
    }, this);

    return true;
}

// close the udp server socket
void UdpReceiver::closeUp() {
    // signal listener thread to stop
    m_threadRunning = false;

    // wait for thread finish
    m_listenerThread.join();

    // close the udp server socket
    close(m_socket);
}
