#ifndef VULKAN_PATHTRACER_VK_ALLOCATOR_H
#define VULKAN_PATHTRACER_VK_ALLOCATOR_H

#include <memory>
#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

#include "context.h"

namespace vpt::vulkan {

class allocator {
public:
    allocator(const vk::raii::Instance &instance,
              const DeviceContext &device_ctx);
    ~allocator();

    allocator(const allocator &) = delete;
    allocator &operator=(const allocator &) = delete;

    [[nodiscard]] VmaAllocator* get() { return &handle; };
private:
    std::unique_ptr<VmaAllocatorCreateInfo> handle_create_info = {};
    std::unique_ptr<VmaVulkanFunctions>     handle_functions   = {};
    VmaAllocator handle = nullptr;
};

} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_ALLOCATOR_H
