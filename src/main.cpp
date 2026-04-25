/*
    File: main.cpp
    This is file contains the top level context of application logic
    written by Elias Geiger
*/

#include "opendtu-interface.hpp"
#include "fsm.hpp"
#include "ModbusClient.hpp"
#include "PsuController.hpp"
#include "ConfigFile.hpp"
#include "Utils.hpp"

// global instances
PsuController psu;
Queue<GridLoadState> cmdQueue;
ModbusClient powermeter;
Logger logger("energy-manager.log");
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
    bool status = cfg.LoadConfig();
    if(!status) {
        // std::cerr << "[Config] Failed to open config.txt file!" << std::endl;
        logger.LogMessage(LogChannel::WARNING, "[Config] Failed to open config.txt file");
        logger.LogMessage(LogChannel::INFO, "[Config] --> using default settings");
    }

    // print out the config variable overview
    cfg.PrintConfig();

    // create opendtu instance
    OpenDtuInterface dtu;

    // attempt to start the PSU controller 
    status = psu.Setup(cfg.GetCanInterfaceName());
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // attempt to start udp receiver to listen for power change messages
    status = powermeter.Setup(cfg.GetPowerMeterModbusIp(), cfg.GetPowerMeterModbusPort());
    if(!status) {
        terminateSignalHandler(EXIT_FAILURE);
    }

    // at last create the FSM and pass references to PSU & DTU
    PVPowerPlantFSM fsm(&dtu, &psu, &powermeter);

    // main application loop in the main thread
    while (!ScheduledClose())
    {
        GridLoadState latestGridLoadState;

        // check for new grid load state on the queue
        if(!cmdQueue.TryPop(latestGridLoadState)) {
            sleep_for(milliseconds(100));       // avoid buisy waiting with small idle
            continue;
        }

        // required measurements from DTU
        dtu.FetchCurrentState();

        // update the fsm
        fsm.Update(latestGridLoadState, dtu.GetBatteryToGridPower(), dtu.GetBatteryVoltage());
    }

    // close up
    terminateSignalHandler(EXIT_SUCCESS);

    return EXIT_SUCCESS;
}

void terminateSignalHandler(int code) {
    // shutdown sockets, threads and queue
    powermeter.Closeup();
    psu.Shutdown();
    cmdQueue.Clear();
    exit(code);
}
