/*
    File: UdpReceiver.h
    Udp Receiver is a very basic UDP server that waits for periodic messages
    that contain only a few values payload

    written by Elias Geiger
*/

#pragma once

// includes
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <iostream>
#include <chrono>

#include "Queue.cpp"

using std::chrono::steady_clock;
using std::chrono::seconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

#define MSGLEN 1024

class UdpReceiver 
{
    int m_socket;
    struct sockaddr_in m_serverAddr;

    std::thread m_listenerThread;
    std::atomic<bool> m_threadRunning;

public:
    UdpReceiver();
    ~UdpReceiver();

    bool setup(short);
    void closeUp();
};