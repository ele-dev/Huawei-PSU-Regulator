cmake_minimum_required(VERSION 3.0)

# Setup the project
project(regulatorApp)

# Set the bin folder
set(EXECUTABLE_OUTPUT_PATH "bin")

# Set the C++ language standard
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

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
)

# Add any additional include directories
include_directories(
    include
)

# Add any external libraries (e.g., wiringPi)
# find_library(WIRINGPI_LIB wiringPi)
find_library(PTHREAD_LIB pthread)

# Create the executable
add_executable(regulatorApp ${SOURCES})

# Link the necessary libraries
target_link_libraries(regulatorApp
    # ${WIRINGPI_LIB}
    ${PTHREAD_LIB}
)
