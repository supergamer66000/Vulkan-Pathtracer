cmake_minimum_required(VERSION 3.15)
project(Vulkan_Pathtracer)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(external)

find_package(Vulkan 1.4 REQUIRED)

add_executable(Vulkan_Pathtracer
    src/main.cpp
    src/engine.cpp
    src/engine.h
    src/util.h
    src/vk_util.h
    src/vk_instance.h
)

target_link_libraries(Vulkan_Pathtracer PRIVATE
    Vulkan::Vulkan glfw glm
)

target_include_directories(Vulkan_Pathtracer PRIVATE src)
