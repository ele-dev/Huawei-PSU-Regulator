cmake_minimum_required(VERSION 3.20)

# Setup the project
project(energy-manager)

# Set the bin folder
set(EXECUTABLE_OUTPUT_PATH "bin")

# Set the C++ language standard
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
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
    src/Logger.cpp
)

# Find curl and modbus library
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_library(PTHREAD_LIB pthread)
find_library(MODBUS_LIB libmodbus)
# find_library(WIRINGPI_LIB wiringPi)             # (raspberry pi only)

# pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

# Create the executable
add_executable(energy-manager ${SOURCES})

target_link_directories(energy-manager PRIVATE
    ${LIBMODBUS_LIBRARY_DIRS}
)

# Link the necessary libraries
target_link_libraries(energy-manager
    # ${WIRINGPI_LIB}                             # (raspberry pi only)
    ${PTHREAD_LIB}
    ${LIBMODBUS_LIBRARIES}
    CURL::libcurl
)

target_include_directories(energy-manager PRIVATE
    ${LIBMODBUS_INCLUDE_DIRS}
)
