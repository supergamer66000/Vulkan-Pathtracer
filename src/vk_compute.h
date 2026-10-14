#ifndef VULKAN_PATHTRACER_VK_COMPUTE_H
#define VULKAN_PATHTRACER_VK_COMPUTE_H

#include <cstdint>
#include <type_traits>
#include <span>
#include <vulkan/vulkan_raii.hpp>

namespace vpt::vulkan {

struct nopush {};

template<typename Constants, uint32_t LocalX, uint32_t LocalY = 1, uint32_t LocalZ = 1>
class compute_pipeline {
    static constexpr bool has_push = !std::is_same_v<Constants, nopush>;

    static_assert(LocalX > 0 && LocalY > 0 && LocalZ > 0, "Local Size needs to be greater than zero");
    static_assert(!has_push || std::is_trivially_copyable_v<Constants>, "Constants must be copyable");
    static_assert(!has_push || (sizeof(Constants) <= 128 || sizeof(Constants) % 4 == 0),
                  "Constants must be in 4 byte intervials, and 128 bytes at most");

public:
    compute_pipeline(
        const vk::raii::Device &device,
        std::span<const uint32_t> spv_binary,
        vk::DescriptorSetLayout set_layout
    );
    explicit compute_pipeline() = default;

    // Get the size of the dispatched groups
    static constexpr vk::Extent2D group_count(vk::Extent2D extent) {
        return {(extent.width + LocalX - 1) / LocalX, (extent.height + LocalY - 1) / LocalY};
    }

    void bind(const vk::raii::CommandBuffer &cmd, vk::DescriptorSet set) const;
    void push(const vk::raii::CommandBuffer &cmd, const Constants &values) const;
    void dispatch(const vk::raii::CommandBuffer &cmd, vk::Extent2D size) const;
private:
    vk::raii::ShaderModule shader = nullptr;
    vk::raii::PipelineLayout layout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;
};

} // namespace vpt::vulkan
#endif //VULKAN_PATHTRACER_VK_COMPUTE_H
