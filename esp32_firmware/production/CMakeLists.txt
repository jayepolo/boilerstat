# ESP-IDF Project CMakeLists.txt file for boilerstat_simulator
cmake_minimum_required(VERSION 3.16)

# Include build configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set project name
set(PROJECT_NAME "boilerstat_simulator")

# Define the project
project(${PROJECT_NAME})