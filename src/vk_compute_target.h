#ifndef VULKAN_PATHTRACER_VK_COMPUTE_TARGET_H
#define VULKAN_PATHTRACER_VK_COMPUTE_TARGET_H

#include <vma/vk_mem_alloc.h>
#include "vk_mem_alloc_raii.hpp"
#include "vk_util.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "vk_descriptors.h"

namespace vpt::vulkan {

class compute_target {
public:
    using layout_t = set_layout<binding<0, vk::DescriptorType::eStorageImage, compute_stage>>;
    static constexpr auto format = vk::Format::eR32G32B32A32Sfloat;

    compute_target(const vk::raii::Device &device, vma::raii::Allocator allocator, vk::Extent2D extent) :
        extent(extent), image(allocator.createImage(vk_util::image::make_info(extent, format), vk_util::image::make_allocation_info())),
        image_view(device, vk_util::image::make_view_info(*image, format)), layout(device), descriptor_set(device, layout) {}

    [[nodiscard]] vk::Extent2D get_extent() { return extent; }
    [[nodiscard]] vk::Image get_image() { return image; }
    [[nodiscard]] const layout_t &get_layout() { return layout; }
    [[nodiscard]] vk::DescriptorSet get_descriptor_set() { return descriptor_set.handle(); }
private:
    vk::Extent2D extent;
    vma::raii::Image image;
    vk::raii::ImageView image_view;
    layout_t layout;
    descriptor_set<layout_t> descriptor_set;
};

} // namespace vpt

#endif //VULKAN_PATHTRACER_VK_COMPUTE_TARGET_H
