/*
    File: config.h
    Here are the default values for all essential configuration variables defined

    written by Elias Geiger
*/

#pragma once

// for debugging purposes
// #define _VERBOSE_OUTPUT

// compile flag for raspberry pi exclusive functionality
#define _TARGET_RASPI

/*
    These are the default fallback values for all config variables.
    They are only used in case the config file doesn't contain valid entries
    don't change anything here! use the config.txt file instead!
*/ 

// configuration parameters
#define CAN_INTERFACE_NAME "can0"
#define UDP_PORT 2000

// desired value for grid import power for charging 
// recommendation: 0 or slightly above
#define TARGET_GRID_POWER 0

// regulator only tries to compensate for errors bigger than this constant
// recommendation: between 5 and 15
#define REGULATOR_ERR_THRESHOLD 7

// enforced wait time until which elapses before next power command is processed in milliseconds
#define REGULATOR_IDLE_TIME 1200

// bounds for min and max DC ouput power of the charger PSU
#define MAX_CHARGE_POWER 700
#define MIN_CHARGE_POWER 50

// absorbtion voltage to use for charging
// recommendation: go lower to spare battery lifetime if you don't need the capacity
#define CHARGER_ABSORPTION_VOLTAGE 52.5f

/// advanced features ------------------------------------------------------------------------------

// automatic close up in at given time (e.g. in the evening right after sunset)
#define SCHEDULED_EXIT_ENABLED false
#define SCHEDULED_EXIT_HOUR 18          // --> at 18:20 local time
#define SCHEDULED_EXIT_MINUTE 22

// automatic slot detect control via GPIO pins (on raspberry pi only)
#define SD_CONTROL_ENABLED false
#define SD_KEEP_ALIVE_TIME 60

