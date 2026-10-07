#include "engine.h"
#include "GLFW/glfw3.h"

#include <iostream>
#include <chrono>
#include <stdexcept>


namespace vpt {
    engine::engine() : 
        width(1280), height(720)
    {
       // Init GLFW //
        if (!glfwInit())
            throw std::runtime_error("Error initializing glfw");

        // glfw Hints
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE , false);

        window = glfwCreateWindow(width, height, "Vulkan Pathtracer", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Error creating glfw window");
        glfwMakeContextCurrent(window);
    }

    void engine::init_vulkan() {

    }

    void engine::start() {
        auto end_time = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            double deltatime = std::chrono::duration<double>(end_time - start_time).count();
            end_time = std::chrono::high_resolution_clock::now();

            std::cout << deltatime << '\n';

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    engine::~engine() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
