#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_allocator.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace vpt::vulkan {
    allocator::allocator(const vk::raii::Instance &instance,
                         const DeviceContext &device_ctx) {
        VmaAllocatorCreateInfo info = {};
        info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        info.vulkanApiVersion = vk::ApiVersion14;
        info.instance         = *instance;
        info.physicalDevice   = **device_ctx.physical_device;
        info.device           = **device_ctx.device;


        VmaVulkanFunctions functions = {};
        functions.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            instance.getProcAddr("vkGetInstanceProcAddr"));
        functions.vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
            device_ctx.device->getProcAddr("vkGetDeviceProcAddr"));

        if (!functions.vkGetInstanceProcAddr)
            throw std::runtime_error("VMA: vkGetInstanceProcAddr is null");
        if (!functions.vkGetDeviceProcAddr)
            throw std::runtime_error("VMA: vkGetDeviceProcAddr is null");

        handle_functions = std::make_unique<VmaVulkanFunctions>(functions);
        info.pVulkanFunctions = handle_functions.get();

        handle_create_info = std::make_unique<VmaAllocatorCreateInfo>(info);

       if (vmaCreateAllocator(handle_create_info.get(), &handle) != VK_SUCCESS)
            throw std::runtime_error("Failed to create the VMA allocator");
    }

    allocator::~allocator() {
        if (handle) vmaDestroyAllocator(handle);
    }
} // namespace vpt::vulkan
