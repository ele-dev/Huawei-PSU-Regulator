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

# Find packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

# Check for required libs (with pkg-config)
pkg_check_modules(LIBMODBUS REQUIRED IMPORTED_TARGET libmodbus)

# Find other libs (without pkg-config)
# find_library(WIRINGPI_LIB wiringPi)             # (raspberry pi only)

# Create the executable
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

# Link the necessary libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # ${WIRINGPI_LIB}                             # (raspberry pi only)
    PkgConfig::LIBMODBUS
    CURL::libcurl
    nlohmann_json::nlohmann_json
)
