cmake_minimum_required(VERSION 3.16)

# Setup the project
project(energy-manager)

# Set the bin folder
set(EXECUTABLE_OUTPUT_PATH "bin")

# Set the C++ language standard
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the Raspberry Pi toolchain file (only for cross compiling)
# set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/RaspberryPi.cmake)

# Add your source files here
set(SOURCES
    src/main.cpp
    src/opendtu-interface.cpp
    src/fsm.cpp
    src/ConfigFile.cpp
    src/PsuController.cpp 
    src/Queue.cpp 
    src/ModbusClient.cpp
    src/Utils.cpp 
)

# Find curl and modbus library
find_package(glog 0.7.1 REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

# Add any additional include directories
include_directories(
    ${LIBMODBUS_INCLUDE_DIRS}
)

link_directories(
    ${LIBMODBUS_LIBRARY_DIRS}
)

# Add any external libraries (e.g., wiringPi)
# find_library(WIRINGPI_LIB wiringPi)             # (raspberry pi only)
find_library(PTHREAD_LIB pthread)
find_library(MODBUS_LIB libmodbus)

# Create the executable
add_executable(energy-manager ${SOURCES})

# Link the necessary libraries
target_link_libraries(energy-manager
    # ${WIRINGPI_LIB}                             # (raspberry pi only)
    ${PTHREAD_LIB}
    ${LIBMODBUS_LIBRARIES}
    CURL::libcurl
    glog::glog
)
