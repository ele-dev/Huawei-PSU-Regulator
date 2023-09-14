/*
    File: config.h
    Here are the essential configuration variables define

    written by Elias Geiger
*/

#pragma once

// for debugging purposes
// #define _VERBOSE_OUTPUT

/*
    These are the default fallback values for all config variables.
    They are only used in case the config file doesn't contain valid entries
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