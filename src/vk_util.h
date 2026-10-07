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

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT,
            const vk::DebugUtilsMessengerCallbackDataEXT* data, void*) {
        std::cerr << "[vk] " << data->pMessage << '\n';
        return vk::False;
    }

    inline std::vector<const char*> get_required_instance_extension_by_glfw() {
        uint32_t glfw_extension_count = 0;
        auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        return std::vector<const char*>{glfw_extensions, glfw_extensions + glfw_extension_count};
    }

} // namespace vpt

#endif //VULKAN_PATHTRACER_VK_UTIL_H
