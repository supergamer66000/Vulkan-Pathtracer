#ifndef VULKAN_PATHTRACER_VK_SWAPCHAIN_H
#define VULKAN_PATHTRACER_VK_SWAPCHAIN_H

#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "context.h"

namespace vpt::vulkan {
    class swapchain {
    public:
        swapchain(const vk::raii::Instance &instance,
                  const DeviceContext &device_ctx,
                  const vk::raii::SurfaceKHR &surface);

        void get_surface();

        void swap_buffers();
    private:
        vk::raii::SwapchainKHR vk_swapchain = nullptr;
        std::vector<vk::Image> swapchain_images;
        vk::Extent2D swapchain_extent;

        vk::SurfaceFormat2KHR surface_format;
    };
} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_SWAPCHAIN_H
