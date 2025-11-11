# Top-level CMakeLists.txt for ESP-IDF project
# This is required by PlatformIO ESP-IDF framework

cmake_minimum_required(VERSION 3.16)

# Include project configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(peristaltic_pump_system)
