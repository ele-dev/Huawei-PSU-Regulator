/*
    File: main.cpp
    The main file contains the algorithm for the actual power regualtion

    written by Elias Geiger
*/

// Includes
#include "UdpReceiver.cpp"
#include "PsuController.cpp"
#include "ConfigFile.h"
#include "Utils.h"
using std::this_thread::sleep_for;

// global instances
PsuController* psu = nullptr;
Queue<short> cmdQueue;
UdpReceiver* rc = nullptr;
ConfigFile cfg("config.txt");

// function prototypes
void terminateSignalHandler(int);
void powerRegulation();
float currentBasedOnPower(float, float);
bool scheduledClose();


// ----- Main Function ----- //
int main(int argc, char **argv) 
{
    time_t currTime = time(NULL);
    tm* tm_local = localtime(&currTime);
    std::cout << "[" << tm_local->tm_hour << ":" << tm_local->tm_min << "] Huawei-PSU-Regulator application launched \n" << std::endl;

    // create signal handler for clean Ctrl+C close up
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = terminateSignalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // read config variables from config file
    bool status = cfg.loadConfig();
    if(!status) {
        std::cerr << "[Config] Failed to open config.txt file!" << std::endl;
        std::cout << " --> using default settings" << std::endl;
    }

    // print out the config variable overview
    cfg.printConfig();

    // create a psu controller instance and store pointer on it globally
    PsuController pc;
    psu = &pc;

    // attempt to start the PSU controller 
    status = pc.setup(cfg.getCanInterfaceName());
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // create udp receiver and store pointer on it globally
    UdpReceiver receiver;
    rc = &receiver;

    // attempt to start udp receiver to listen for power change events
    status = receiver.setup(cfg.getUdpPort());
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    sleep_for(milliseconds(2200));          // wait a bit 
    std::cout << "[Main] Setup completed" << std::endl;

    powerRegulation();

    // wait for a key press to exit application
    std::cin.get();

    // Close up
    receiver.closeUp();
    pc.shutdown();
    cmdQueue.clear();

    return EXIT_SUCCESS;
}

void terminateSignalHandler(int code) {
    // shutdown sockets, threads and queue
    if(psu != nullptr) {
        psu->shutdown();
    }
    if(rc != nullptr) {
        rc->closeUp();
    }
    
    exit(code);
}

void powerRegulation() {
    short latestPowerState = 0;
    short error = 0;

    // main loop of the power regulator
    while(!scheduledClose()) 
    {
        // check for new command on the queue
        if(!cmdQueue.tryPop(latestPowerState)) {
            sleep_for(milliseconds(100));       // avoid buisy waiting with small idle
            continue;
        }

        // calculate error (absolute difference from target value)
        // don't try to compensate for very small errors
        error = cfg.getTargetGridPower() - latestPowerState;
        if(abs(error) < cfg.getRegulatorErrorThreshold()) {
            continue;
        }
        short powerCmd = static_cast<short>(psu->getCurrentIntputPower()) + error;

        // set bounds for allowed power commands (min and max)
        if(powerCmd > cfg.getMaxChargePower()) {
            powerCmd = cfg.getMaxChargePower();
        }

        if(powerCmd < cfg.getMinChargePower()) {
            powerCmd = 0;
        }

        // translate power command into max current command. use current output voltage for calculation
        float maxCurrentCmd = currentBasedOnPower(static_cast<float>(powerCmd), psu->getCurrentOutputVoltage());

        // send max current command to the PSU and idle a short time 
        psu->setMaxCurrent(maxCurrentCmd, false);
        sleep_for(milliseconds(cfg.getRegulatorIdleTime()));
    }
}

// Helper function to round float values on decimals
float round(float var)
{
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

float currentBasedOnPower(float power, float batteryVoltage) {
    // Determine expected AC/DC conversion efficiency based on power command
    float eff = 0.0f;
    if(power >= 1 && power < 461) {
        eff = 0.88f;
    } else if(power >= 461 && power < 704) {
        eff = 0.937f;
    } else if(power >= 704 && power < 1050) {
        eff = 0.952f;
    } else if(power >= 1050) {
        eff = 0.96f;
    }

    // calculate and round the current 
    float result = round(0.9876f * eff * power / batteryVoltage);

    return result;
}


