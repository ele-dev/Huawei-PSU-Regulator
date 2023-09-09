/// --------------- main.cpp -------------------- //

// includes
#include "UdpReceiver.cpp"
#include "PsuController.cpp"
#include "config.h"

using std::this_thread::sleep_for;

// global instances
PsuController* psu = nullptr;
Queue<short> cmdQueue;
UdpReceiver* rc = nullptr;

// prototypes
void terminateSignalHandler(int);
void powerRegulation();
float currentBasedOnPower(float, float);

// ----- Main Function ----- //
int main(int argc, char **argv) 
{
    std::cout << "Application launch -----------" << std::endl;

    // create signal handler for clean Ctrl+C close up
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = terminateSignalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // create a psu controller instance and store pointer on it globally
    PsuController pc;
    psu = &pc;

    // attempt to start the PSU controller 
    bool status = pc.setup(CAN_INTERFACE_NAME);
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // create udp receiver and store pointer on it globally
    UdpReceiver receiver;
    rc = &receiver;

    // attempt to start udp receiver to listen for power change events
    status = receiver.setup();
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
    while(true) 
    {
        // check for now command on the queue
        if(!cmdQueue.tryPop(latestPowerState) && abs(error < 15)) {
            sleep_for(milliseconds(100));
            continue;
        }

        // calculate error (absolute difference from target value)
        error = TARGET_GRID_POWER - latestPowerState;
        if(abs(error) < 8) {
            continue;
        }
        short powerCmd = static_cast<short>(psu->getCurrentIntputPower()) + error;
        // std::cout << "[Regulator]: Error of " << error << "W" << std::endl;
        // std::cout << "[Regulator]: suggested reaction with " << powerCmd << "W command" << std::endl;

        // set bounds for allowe power commands (min and max)
        if(powerCmd > MAX_CHARGE_POWER) {
            powerCmd = MAX_CHARGE_POWER;
        }

        if(powerCmd < MIN_CHARGE_POWER) {
            powerCmd = 0;
        }

        // translate power command into max current command. use current output voltage for calculation
        float maxCurrentCmd = currentBasedOnPower(static_cast<float>(powerCmd), psu->getCurrentOutputVoltage());

        // send max current command to the PSU and idle a short time 
        psu->setMaxCurrent(maxCurrentCmd, false);
        sleep_for(milliseconds(1500));
    }
}

// Helper function to round float values on decimals
float round(float var)
{
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

float currentBasedOnPower(float power, float batteryVoltage) {
    // Determine expected AC/DC conversion efficiency based on power
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

