cmake_minimum_required(VERSION 3.12)
project(cl3w)

find_package(OpenCL REQUIRED)

add_library(cl3w STATIC "src/cl3w.c" "include/cl3w.h")
target_include_directories(cl3w PUBLIC "include" ${OpenCL_INCLUDE_DIR})

add_subdirectory(test)
