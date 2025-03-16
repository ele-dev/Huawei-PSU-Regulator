# Huawei-PSU-Regulator

## The Idea
An application implemented as finite state machine that controls the swiching between charging and discharging of the battery, based on the current demand of the household --> combination of zero feedback & adaptive charge. 
The system is centered around existing subsystems (Tasmota Energy Meter, OpenDTU-on-Battery, Huawei-PSU-Regulator) and combines them with into a smart centralized energy-manager to use self produced energy as efficient as possible.

## System components
- A Shelly Pro 3em energy meter with MODBUS/TCP enabled
- A Huawei R4850G2 rectifier power supply with CAN interface for remote control
- A linux based computer with a working CAN interface (Socket CAN) connected to the Power supply (recommendation: D-SUB USB-2-CAN module + Raspberry Pi 4B)
- A working 48V battery system that can be charged with the Huawei R4850G2 power supply (e.g. Pylontech US2000/3000/5000 or custom 48V DIY Battery packs)
- An ESP32 running OpenDTU-on-Battery with control over the battery-to-grid inverter and connection to the Energy Meter

## Quick setup
1. Download the latest release (zip archive) on your target system and extract it
2. Enable MODBUS/TCP in webinterface of Shelly Pro 3em
3. Use the config.txt file to set your settings for regulator, opendtu, etc.
4. Execute the regulator binary file on the terminal 

## Build & Run on linux system
1. Install system dependencies ``` sudo apt install pkg-config libmodbus-dev libcurl4-openssl-dev nlohmann-json3-dev cmake g++ build-essential ```
2. Clone the repository on the linux system that is connected to the power supply via CAN
3. Creat a ``` build ```folder and run ``` cmake .. ``` and ``` make ``` from there to build an application binary
4. Customize your runtime settings in config.txt file
5. Execute the command line application in the bin folder with ``` ./energy-manager ``` (use ``` screen -dmS energy-manager ./energy-manager ``` to run detached screen)
   
## Acknowledgements
The code for the CAN commuication was based on work from craigpeacock
https://github.com/craigpeacock/Huawei_R4850G2_CAN



