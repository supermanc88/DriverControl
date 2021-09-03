cmake_minimum_required(VERSION 3.0)

project(DriverControl)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(DRIVER_CONTROL_SRC main.cpp driver_control.h driver_control.cpp)

add_executable(${PROJECT_NAME} ${DRIVER_CONTROL_SRC})