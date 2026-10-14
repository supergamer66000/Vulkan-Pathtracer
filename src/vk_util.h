#ifndef VULKAN_PATHTRACER_VK_UTIL_H
#define VULKAN_PATHTRACER_VK_UTIL_H

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include <vector>
#include <algorithm>
#include <cstring>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

namespace vpt::vk_util {

    namespace image {
        inline vk::ImageCreateInfo make_info(vk::Extent2D size, vk::Format format) {
            vk::ImageCreateInfo info{};
            info.imageType = vk::ImageType::e2D;
            info.format = format;
            info.extent = vk::Extent3D{size.width, size.height, 1};
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = vk::SampleCountFlagBits::e1;
            info.tiling = vk::ImageTiling::eOptimal;
            info.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc;
            info.sharingMode = vk::SharingMode::eExclusive;
            info.initialLayout = vk::ImageLayout::eUndefined;
            return info;
        }

        inline vk::ImageViewCreateInfo make_view_info(vk::Image image, vk::Format format) {
            vk::ImageViewCreateInfo info{};
            info.image = image;
            info.viewType = vk::ImageViewType::e2D;
            info.format = format;
            info.subresourceRange = vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
            };
            return info;
        }

        inline vma::AllocationCreateInfo make_allocation_info() {
            vma::AllocationCreateInfo info{};
            info.usage = vma::MemoryUsage::eAutoPreferDevice;
            info.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
            return info;
        }
    }

    template <typename RequiredRange, typename PropertyRange, typename NameGetter>
    inline auto find_unsupported(
            const RequiredRange& required,
            const PropertyRange& supported,
            NameGetter&& get_name) {
        return std::ranges::find_if(required,
            [&supported, &get_name](const auto& required_name) {
                return std::ranges::none_of(supported,
                    [&required_name, &get_name](const auto& property) {
                        return std::strcmp(get_name(property), required_name) == 0;
                    });
            });
    }

    inline void transition_image(const vk::raii::CommandBuffer &cmd, const vk::Image image,
                                 const vk::ImageLayout old_layout, const vk::ImageLayout new_layout,
                                 const vk::PipelineStageFlags2 src_stage, const vk::AccessFlags2 src_access,
                                 const vk::PipelineStageFlags2 dst_stage, const vk::AccessFlags2 dst_access) {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.srcStageMask = src_stage;
        barrier.srcAccessMask = src_access;
        barrier.dstStageMask = dst_stage;
        barrier.dstAccessMask = dst_access;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        vk::DependencyInfo dependency{};
        dependency.setImageMemoryBarriers(barrier);
        cmd.pipelineBarrier2(dependency);
    }

    inline std::vector<const char*> get_required_instance_extension_by_glfw() {
        uint32_t glfw_extension_count = 0;
        auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        return std::vector<const char*>{glfw_extensions, glfw_extensions + glfw_extension_count};
    }

} // namespace vpt

#endif //VULKAN_PATHTRACER_VK_UTIL_H
