#include "opendtu-interface.h"
#include "fsm.h"

#include "ModbusClient.h"
#include "PsuController.h"
#include "ConfigFile.h"
#include "Utils.h"

// global instances
PsuController psu;
Queue<GridLoadState> cmdQueue;
ModbusClient powermeter;
ConfigFile cfg("config.txt");

// function prototypes
void terminateSignalHandler(int);

int main(int argc, char **argv)
{
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

    // create opendtu instance
    OpenDtuInterface dtu;

    // attempt to start the PSU controller 
    status = psu.setup(cfg.getCanInterfaceName());
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // attempt to start udp receiver to listen for power change messages
    // status = receiver.setup(cfg.getUdpPort());
    status = powermeter.setup("Shelly-IP", SHELLY_MODBUS_PORT);
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // at last create the FSM and pass references to PSU & DTU
    PVPowerPlantFSM fsm(&dtu, &psu);

    // main application loop in the main thread
    while (!scheduledClose())
    {
        GridLoadState latestGridLoadState;

        // check for new grid load state on the queue
        if(!cmdQueue.tryPop(latestGridLoadState)) {
            sleep_for(milliseconds(100));       // avoid buisy waiting with small idle
            continue;
        }

        // required measurements from DTU
        dtu.fetchCurrentState();

        #ifdef _VERBOSE_OUTPUT
            std::cout << "Grid Load:         " << latestGridLoadState.tasmotaPowerCmd << "W" << std::endl;
            std::cout << "AC Charger Power:  " << latestGridLoadState.psuAcInputPower << "W" << std::endl;
            std::cout << "AC Inverter Power: " << dtu.getBatteryToGridPower() << "W" << std::endl;
            std::cout << "Battery Voltage:   " << dtu.getBatteryVoltage() << "V" << std::endl;
        #endif

        // update the fsm
        fsm.update(latestGridLoadState, dtu.getBatteryToGridPower(), dtu.getBatteryVoltage());
    }

    // close up
    terminateSignalHandler(EXIT_SUCCESS);

    return EXIT_SUCCESS;
}

void terminateSignalHandler(int code) {
    // shutdown sockets, threads and queue
    powermeter.closeup();
    psu.shutdown();
    cmdQueue.clear();
    exit(code);
}