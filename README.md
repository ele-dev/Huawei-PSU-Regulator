# Huawei-PSU-Regulator

## The Idea
Dynamically regulate the power output of a Huawei R4850G2 power supply via CAN interface from a linux based system (e.g. raspberry pi)
to charge a battery in a way that almost no energy is fed back to the grid for free in periods of overproduction.
This software aims to store 

## System components
- Tasmota with Smart Energy Meter sensor flashed on a ESP32 or an equivalent energy meter device with support for custom scripting (sending of UDP messages)
- A Huawei R4850G2 rectifier power supply with CAN interface for remote control
- A linux based computer with a working CAN interface (Socket CAN) connected to the Power supply (recommendation: Raspberry Pi with CAN HAT)
- A working 48V battery system that can be charged with the Huawei R4850G2 power supply (e.g. Pylontech US2000/3000/5000 or custom DIY Battery packs)

## Quick setup
1. clone the repository on the linux system that is connected to the power supply via CAN
2. go into the src folder and run ``` make clean and make ``` 
4. execute the command line application in the bin folder with ``` ./regulatorApp ``` (use ``` screen -dmS regualtor ./regulatorApp ``` to run detached screen)

## Acknowledgements
The code for the CAN commuication was based on work from craigpeacock
https://github.com/craigpeacock/Huawei_R4850G2_CAN



