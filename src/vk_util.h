#ifndef VULKAN_PATHTRACER_VK_UTIL_H
#define VULKAN_PATHTRACER_VK_UTIL_H

#include "GLFW/glfw3.h"
#include <vector>
#include <algorithm>
#include <cstring>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <iostream>

namespace vpt::vk_util {

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
