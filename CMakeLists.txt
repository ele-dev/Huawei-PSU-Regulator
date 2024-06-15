cmake_minimum_required(VERSION 3.10)

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
    src/PsuController.cpp
    src/Queue.cpp 
    src/UdpReceiver.cpp 
    src/Utils.cpp
    src/ConfigFile.cpp 
    src/fsm.cpp
)

# Add any additional include directories
include_directories(
    include
)

# Find curl library
find_package(CURL REQUIRED)

# Add any external libraries (e.g., wiringPi)
find_library(WIRINGPI_LIB wiringPi)             # (raspberry pi only)
find_library(PTHREAD_LIB pthread)

# Create the executable
add_executable(energy-manager ${SOURCES})

# Link the necessary libraries
target_link_libraries(energy-manager
    ${WIRINGPI_LIB}                             # (raspberry pi only)
    ${PTHREAD_LIB}
    CURL::libcurl
)
