#include "vk_image.h"

namespace vpt::vulkan {
    image::image(VmaAllocator allocator, const vk::ImageCreateInfo &info)
        : vma(allocator), image_format(info.format), image_extent(info.extent) {
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        const VkImageCreateInfo c_info = info;
        VkImage raw = VK_NULL_HANDLE;
        if (vmaCreateImage(vma, &c_info, &alloc_info, &raw, &allocation, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image through VMA");
        vk_image = vk::Image(raw);
    }

    image::~image() {
        if (vk_image) vmaDestroyImage(vma, static_cast<VkImage>(vk_image), allocation);
    }
} // namespace vpt::vulkan

