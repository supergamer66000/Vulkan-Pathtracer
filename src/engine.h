#ifndef VULKAN_PATHTRACER_ENGINE_H
#define VULKAN_PATHTRACER_ENGINE_H

#include "vulkan/vulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <vulkan/vulkan_raii.hpp>

namespace vpt {
    class engine {
    public:
        engine();
        ~engine();

        void start();

    private:
        void init_glfw();

        struct GLFWExtensionSupportedInfo {
            uint32_t extension_count;
            const char** extensions = nullptr;
        };
        GLFWExtensionSupportedInfo get_glfw_extensions() {
            uint32_t glfw_extension_count = 0;
            auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

            auto ext_prop = vk_context.enumerateInstanceExtensionProperties();
        
            for (uint32_t i = 0; i < glfw_extension_count; ++i) {
                if (std::ranges::none_of(ext_prop, [glfwExtension = glfw_ext[i]](auto const& extensionProperty) {
                    return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
                }))
                    throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_ext[i]));
            }
            return {glfw_extension_count, glfw_ext};
        }
        const char* WINDOW_NAME = "Vulkan Pathtracer";
        GLFWwindow* window;
        uint16_t width, height;
        bool running = true;

        const std::vector<const char*> enabled_validation_layers = {
           "VK_LAYER_KHRONOS_validation" 
        };
        static const bool use_validation_layers = true;

        void init_vulkan_instance();
        void init_vulkan_devices();

        vk::raii::Context vk_context;
        vk::raii::Instance vk_instance                               = nullptr;
        vk::raii::DebugUtilsMessengerEXT vk_debug                    = nullptr;
        std::unique_ptr<vk::raii::PhysicalDevice> vk_physical_device = nullptr;
        std::unique_ptr<vk::raii::Device> vk_device                  = nullptr;
    };
};

#endif //VULKAN_PATHTRACER_ENGINE_H
