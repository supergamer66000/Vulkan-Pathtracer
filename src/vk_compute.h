#ifndef VULKAN_PATHTRACER_VK_COMPUTE_H
#define VULKAN_PATHTRACER_VK_COMPUTE_H

#include "vulkan/vulkan.hpp"
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
    ) {
        vk::ShaderModuleCreateInfo shader_info{};
        shader_info.setCodeSize(spv_binary.size_bytes())
            .setPCode(spv_binary.data());
        shader = device.createShaderModule(shader_info);

        const vk::PushConstantRange push_range {
            vk::ShaderStageFlagBits::eCompute, 0, sizeof(Constants)
        };
        vk::PipelineLayoutCreateInfo layout_info{};
        layout_info.setSetLayouts(set_layout);
        if constexpr (has_push)
            layout_info.setPushConstantRanges(push_range);
        layout = vk::raii::PipelineLayout(device, layout_info);

        constexpr uint32_t word = sizeof(uint32_t);
        const std::array<vk::SpecializationMapEntry, 3> entries{
            vk::SpecializationMapEntry{0, 0 * word, word},
            vk::SpecializationMapEntry{1, 1 * word, word},
            vk::SpecializationMapEntry{2, 2 * word, word},
        };
        const std::array<uint32_t, 3> local_size{LocalX, LocalY, LocalZ};
        vk::SpecializationInfo specialization{};
        specialization.setMapEntries(entries);
        specialization.dataSize = sizeof(local_size);
        specialization.pData = local_size.data();

        vk::PipelineShaderStageCreateInfo stage{};
        stage.stage = vk::ShaderStageFlagBits::eCompute;
        stage.module = *shader;
        stage.pName = "main";
        stage.pSpecializationInfo = &specialization;

        vk::ComputePipelineCreateInfo pipeline_info{};
        pipeline_info.stage = stage;
        pipeline_info.layout = *layout;
        pipeline = vk::raii::Pipeline(device, nullptr, pipeline_info);
    }
    explicit compute_pipeline() = default;

    // Get the size of the dispatched groups
    static constexpr vk::Extent2D group_count(vk::Extent2D extent) {
        return {(extent.width + LocalX - 1) / LocalX, (extent.height + LocalY - 1) / LocalY};
    }

    void bind(const vk::raii::CommandBuffer &cmd, vk::DescriptorSet set) const {
        const auto bind_point = vk::PipelineBindPoint::eCompute;
        cmd.bindPipeline(bind_point, *pipeline);
        cmd.bindDescriptorSets(bind_point, *layout, 0, set, {});
    }

    void push(const vk::raii::CommandBuffer &cmd, const Constants &values) const 
        requires(has_push) {
        cmd.pushConstants<Constants>(layout, vk::ShaderStageFlagBits::eCompute, 0, values);
    }

    void dispatch(const vk::raii::CommandBuffer &cmd, vk::Extent2D size) const {
        cmd.dispatch(size.width, size.height, 1);
    }
private:
    vk::raii::ShaderModule shader = nullptr;
    vk::raii::PipelineLayout layout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;
};

} // namespace vpt::vulkan
#endif //VULKAN_PATHTRACER_VK_COMPUTE_H
