# Huawei-PSU-Regulator

## The Idea
Dynamically regulate the power output of a Huawei PSU via CAN interface from a linux based system (e.g. raspberry pi)
to charge a battery in a way that almost no energy is fed back to the grid in periods of overproduction

## System components
- Tasmota Energy Smart Meter flashed on ESP32 or equivalent energy meter device with support for custom scripting (UDP client)
- A Huawei AC/DC Power supply with CAN interface for remote control
- A linux based computer with a working CAN interface (Socket CAN) connected to the Power supply (recommendation: Raspberry Pi with CAN HAT)
- A working 48V battery system that can be charged with the Huawei power supply


