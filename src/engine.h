#ifndef VULKAN_PATHTRACER_ENGINE_H
#define VULKAN_PATHTRACER_ENGINE_H

#include "vk_allocator.h"
#include "vk_swapchain.h"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <vulkan/vulkan_raii.hpp>

#include "context.h"
#include "vk_allocator.h"

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
        GLFWExtensionSupportedInfo get_glfw_extensions();
        const char* WINDOW_NAME = "Vulkan Pathtracer";
        GLFWwindow* window;
        uint16_t width, height;
        bool running = true;

        const std::vector<const char*> enabled_validation_layers = {
           "VK_LAYER_KHRONOS_validation",
        };
        static const bool use_validation_layers = true;

        const std::vector<const char*> required_device_extensions = {
            vk::KHRSwapchainExtensionName
        };

        void init_vulkan_instance();
        void init_vulkan_physical_devices();
        void init_vulkan_device();

        /* Quick way to create the glfw sruface */
        static std::optional<vk::raii::SurfaceKHR> create_surface(const GLFWwindow* window, const vk::raii::Instance &instance) {
            VkSurfaceKHR surface;
            if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), const_cast<GLFWwindow*>(window), nullptr, &surface) != VK_SUCCESS) {
                return std::nullopt;
            }
            return vk::raii::SurfaceKHR(instance, surface);
        }
        void init_vulkan_swapchain();
        void init_vulkan_sync();

        vk::raii::Context vk_context;
        std::unique_ptr<vk::raii::Instance> vk_instance              = nullptr;
        vk::raii::DebugUtilsMessengerEXT vk_debug                    = nullptr;
        vulkan::DeviceContext vk_device_ctx{};
        uint32_t vk_graphics_queue_family_index                      = UINT32_MAX;
        std::unique_ptr<vk::raii::Queue> vk_graphics_queue           = nullptr;
        std::unique_ptr<vk::raii::CommandPool> vk_command_pool       = nullptr;
        std::unique_ptr<vk::raii::CommandBuffers> vk_command_buffers = nullptr;

        vk::raii::SurfaceKHR vk_surface                              = nullptr;
        std::unique_ptr<vulkan::swapchain> vk_swapchain              = nullptr;

        static constexpr uint8_t BUFFER_FRAME_COUNT = 2;
        struct FrameData {
            vk::raii::Semaphore available_images;
            vk::raii::Fence frames_in_flight;
        };
        std::vector<FrameData> frame_data;

        void draw_frame();
        std::vector<vk::raii::Semaphore> vk_render_finished;
        uint32_t current_frame = 0;
        std::unique_ptr<vulkan::allocator> vk_allocator = nullptr;
    };
};

#endif //VULKAN_PATHTRACER_ENGINE_H
