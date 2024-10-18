cmake_minimum_required(VERSION 3.16)

# Setup the project
project(energy-manager)

# Set the bin folder
set(EXECUTABLE_OUTPUT_PATH "bin")

# Set the C++ language standard
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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
find_package(nlohmann_json REQUIRED)
find_package(glog REQUIRED)
find_package(CURL REQUIRED)
find_package(libmodbus REQUIRED)

# Add any external libraries (e.g., wiringPi)
# find_library(WIRINGPI_LIB wiringPi)             # (raspberry pi only)
find_library(PTHREAD_LIB pthread)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link the necessary libraries
target_link_libraries(${PROJECT_NAME}
    # ${WIRINGPI_LIB}                             # (raspberry pi only)
    ${PTHREAD_LIB}
    libmodbus::libmodbus
    CURL::libcurl
    glog::glog
    nlohmann_json::nlohmann_json
)
