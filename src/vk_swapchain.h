#ifndef VULKAN_PATHTRACER_VK_SWAPCHAIN_H
#define VULKAN_PATHTRACER_VK_SWAPCHAIN_H

#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "context.h"

namespace vpt::vulkan {
    class swapchain {
    public:
        swapchain(const DeviceContext &device_ctx,
                  const vk::raii::SurfaceKHR &surface,
                  vk::Extent2D frame_buffer_size,
                  uint32_t queue_family);


        [[nodiscard]] const vk::raii::SwapchainKHR &handle() const { return vk_swapchain; }
        [[nodiscard]] const std::vector<vk::Image> &images() const { return swapchain_images; }
        [[nodiscard]] const std::vector<vk::raii::ImageView> &image_views() const { return swapchain_image_views; }
        [[nodiscard]] vk::Format format() const { return surface_format.format; }
        [[nodiscard]] vk::Extent2D extent() const { return swapchain_extent; }

    private:
        static vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR> &formats);
        static vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR> &modes);
        static vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities,
                                          vk::Extent2D framebuffer_extent);

        vk::raii::SwapchainKHR vk_swapchain = nullptr;
        std::vector<vk::Image> swapchain_images;
        std::vector<vk::raii::ImageView> swapchain_image_views;
        vk::Extent2D swapchain_extent;
        vk::SurfaceFormatKHR surface_format;
    };
} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_SWAPCHAIN_H
