#ifndef VULKAN_PATHTRACER_VK_IMAGE_H
#define VULKAN_PATHTRACER_VK_IMAGE_H

#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

namespace vpt::vulkan {

class image {
public:
    image(VmaAllocator allocator, const vk::ImageCreateInfo &info);
    ~image();

    image(const image &) = delete;
    image &operator=(const image &) = delete;
    image(image &&) = delete;
    image &operator=(image &&) = delete;

    [[nodiscard]] vk::Image handle() const { return vk_image; }
    [[nodiscard]] vk::Format format() const { return image_format; }
    [[nodiscard]] vk::Extent3D extent() const { return image_extent; }
private:
    VmaAllocator vma = nullptr;
    VmaAllocation allocation = nullptr;
    vk::Image vk_image;
    vk::Format image_format{};
    vk::Extent3D image_extent{};
};

} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_IMAGE_H
