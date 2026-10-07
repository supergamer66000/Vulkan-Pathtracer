#ifndef VULKAN_PATHTRACER_ENGINE_H
#define VULKAN_PATHTRACER_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>

namespace vpt {
    class engine {
    public:
        engine();
        ~engine();

        void start();

    private:
        GLFWwindow* window;
        uint16_t width, height;
        bool running = true;

        void init_vulkan();
        vk::raii::Context vk_context;
    };
};

#endif //VULKAN_PATHTRACER_ENGINE_H
