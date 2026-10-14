#include "vk_compute.h"

namespace vpt::vulkan {

    template<typename Constants, uint32_t LocalX, uint32_t LocalY, uint32_t LocalZ>
    compute_pipeline<Constants, LocalX, LocalY, LocalZ>::compute_pipeline(
        const vk::raii::Device &device,
        std::span<const uint32_t> spv_binary,
        vk::DescriptorSetLayout set_layout
    ) {
        vk::ShaderModuleCreateInfo shader_info{};
        shader_info.setCodeSize(spv_binary.size_bytes())
            .setPCode(spv_binary.data());

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

    template<typename Constants, uint32_t LocalX, uint32_t LocalY, uint32_t LocalZ>
    void compute_pipeline<Constants, LocalX, LocalY, LocalZ>::bind(const vk::raii::CommandBuffer &cmd, vk::DescriptorSet set) const {
        const auto bind_point = vk::PipelineBindPoint::eCompute;
        cmd.bindPipeline(bind_point, *pipeline);
        cmd.bindDescriptorSets(bind_point, *layout, 0, set, {});
    }

    template<typename Constants, uint32_t LocalX, uint32_t LocalY, uint32_t LocalZ>
    void compute_pipeline<Constants, LocalX, LocalY, LocalZ>::push(const vk::raii::CommandBuffer &cmd, const Constants &values) const 
        requires(has_push) {
        cmd.pushConstants<Constants>(layout, vk::ShaderStageFlagBits::eCompute, 0, values);
    }

    template<typename Constants, uint32_t LocalX, uint32_t LocalY, uint32_t LocalZ>
    void compute_pipeline<Constants, LocalX, LocalY, LocalZ>::dispatch(const vk::raii::CommandBuffer &cmd, vk::Extent2D size) const {
        cmd.dispatch(size.width, size.height, 1);
    }
} // namespace vpt::vulkan

