#include "vk_swapchain.h"

namespace vpt::vulkan {
    swapchain::swapchain(const DeviceContext &device_ctx,
                         const vk::raii::SurfaceKHR &surface,
                         vk::Extent2D frame_buffer_size,
                         uint32_t queue_family) 
    {
        const auto &physical_device = *device_ctx.physical_device;
        const auto &device = *device_ctx.device;

        const auto capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
        const auto formats = physical_device.getSurfaceFormatsKHR(*surface);
        const auto present_modes = physical_device.getSurfacePresentModesKHR(*surface);

        surface_format = choose_surface_format(formats);
        const auto present_mode = choose_present_mode(present_modes);
        swapchain_extent = choose_extent(capabilities, frame_buffer_size);

        uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            image_count = std::min(image_count, capabilities.maxImageCount);

        vk::SwapchainCreateInfoKHR info{};
        info.surface = *surface;
        info.minImageCount = image_count;
        info.imageFormat = surface_format.format;
        info.imageColorSpace = surface_format.colorSpace;
        info.imageExtent = swapchain_extent;
        info.imageArrayLayers = 1;
        info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment
                        | vk::ImageUsageFlagBits::eTransferDst;
        info.imageSharingMode = vk::SharingMode::eExclusive;
        info.preTransform = capabilities.currentTransform;
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        info.presentMode = present_mode;
        info.clipped = vk::True;
        info.oldSwapchain = nullptr;

        vk_swapchain = vk::raii::SwapchainKHR(device, info);
        swapchain_images = vk_swapchain.getImages();

        for (const auto image : swapchain_images) {
            vk::ImageViewCreateInfo view_info{};
            view_info.image = image;
            view_info.viewType = vk::ImageViewType::e2D;
            view_info.format = surface_format.format;
            view_info.subresourceRange = vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
            };
            swapchain_image_views.emplace_back(device, view_info);
        }
    }

    vk::SurfaceFormatKHR swapchain::choose_surface_format(const std::vector<vk::SurfaceFormatKHR> &formats) {
        for (const auto &f : formats) {
            if (f.format == vk::Format::eB8G8R8A8Srgb &&
                f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                return f;
        }
        return formats.front();
    }

    vk::PresentModeKHR swapchain::choose_present_mode(const std::vector<vk::PresentModeKHR> &modes) {
        if (std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end())
            return vk::PresentModeKHR::eMailbox;
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D swapchain::choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities,
                                          const vk::Extent2D framebuffer_extent) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        return {
            std::clamp(framebuffer_extent.width,
                       capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(framebuffer_extent.height,
                       capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }
} // namespace vpt::vulkan
