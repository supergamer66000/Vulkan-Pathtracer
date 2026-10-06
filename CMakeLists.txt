cmake_minimum_required(VERSION 3.15)
project(Vulkan_Pathtracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Vulkan_Pathtracer 
    src/main.cpp
    src/engine.cpp
    src/engine.h
)
