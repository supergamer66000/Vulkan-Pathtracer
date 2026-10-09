#ifndef VULKAN_PATHTRACER_CONTEXT_H
#define VULKAN_PATHTRACER_CONTEXT_H

#include <vulkan/vulkan_raii.hpp>

namespace vpt::vulkan {
    struct DeviceContext {
         std::unique_ptr<vk::raii::PhysicalDevice> physical_device = nullptr;
         std::unique_ptr<vk::raii::Device> device                  = nullptr;
    };
}
#endif //VULKAN_PATHTRACER_CONTEXT_H
