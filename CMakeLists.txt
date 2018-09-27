cmake_minimum_required(VERSION 3.9)
set(CMAKE_C_STANDARD 11)

project(mexData_demo C)

add_executable(mexData_demo main.c mexData.c mexData.h)
