# Huawei-PSU-Regulator

## The Idea
Dynamically regulate the power output of a Huawei R4850G2 power supply via CAN interface from a linux based system (e.g. raspberry pi)
to charge a battery in a way so that almost no energy is fed back to the grid for free in periods of overproduction.

## System components
- Tasmota with Smart Energy Meter sensor flashed on a ESP32 or an equivalent energy meter device with support for custom scripting (sending of UDP messages)
- A Huawei R4850G2 rectifier power supply with CAN interface for remote control
- A linux based computer with a working CAN interface (Socket CAN) connected to the Power supply (recommendation: Raspberry Pi with CAN HAT)
- A working 48V battery system that can be charged with the Huawei R4850G2 power supply (e.g. Pylontech US2000/3000/5000 or custom 48V DIY Battery packs)

## Quick setup
1. Download the latest release (zip archive) on your target system and extract it
2. Enter IP address in berry script, upload in tasmota root directory and restart tasmota
3. Use the config.txt file to change runtime configuration of the regulator app
4. Execute the regulator binary file on the terminal 

Note: "Power_curr" in Berry script has to be adjusted to work with your smart meter interface setup

## Build & Run on linux system
1. Clone the repository on the linux system that is connected to the power supply via CAN
2. Run ``` cmake . ``` and ``` make ``` in the project root directory to build an application binary
3. Customize your runtime settings in bin/config.txt file
4. Execute the command line application in the bin folder with ``` ./regulatorApp ``` (use ``` screen -dmS regualtor ./regulatorApp ``` to run detached screen)
   
## Acknowledgements
The code for the CAN commuication was based on work from craigpeacock
https://github.com/craigpeacock/Huawei_R4850G2_CAN



