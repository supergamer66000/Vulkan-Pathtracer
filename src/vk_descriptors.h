#ifndef VULKAN_PATHTRACER_VK_DESCRIPTORS_H
#define VULKAN_PATHTRACER_VK_DESCRIPTORS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <vulkan/vulkan_raii.hpp>

namespace vpt::vulkan {

inline constexpr uint32_t compute_stage = static_cast<uint32_t>(vk::ShaderStageFlagBits::eCompute);

template <uint32_t Index, vk::DescriptorType Type, uint32_t Stages, uint32_t Count = 1>
struct binding {
    static constexpr uint32_t index = Index;
    static constexpr vk::DescriptorType type = Type;
    static constexpr uint32_t count = Count;
 
    static vk::DescriptorSetLayoutBinding description() {
        return vk::DescriptorSetLayoutBinding{
            Index, Type, Count,
            vk::ShaderStageFlags{static_cast<vk::ShaderStageFlags::MaskType>(Stages)}
        };
    }
};

template <typename... Bindings>
class set_layout {
public:
    explicit set_layout(const vk::raii::Device &device) {
        static_assert(sizeof...(Bindings) > 0, "set_layout needs at least one binding");
        static_assert(unique_indices(), "duplicate binding index in set_layout");
 
        const auto bindings = layout_bindings();
        vk::DescriptorSetLayoutCreateInfo info{};
        info.setBindings(bindings);
        layout = vk::raii::DescriptorSetLayout(device, info);
    }
 
    [[nodiscard]] vk::DescriptorSetLayout handle() const { return *layout; }
 
    static auto layout_bindings() {
        return std::array<vk::DescriptorSetLayoutBinding, sizeof...(Bindings)>{Bindings::description()...};
    }
 
    static auto pool_sizes() {
        return std::array<vk::DescriptorPoolSize, sizeof...(Bindings)>{
            vk::DescriptorPoolSize{Bindings::type, Bindings::count}...
        };
    }
 
    template <uint32_t Index>
    static constexpr bool has_binding() {
        return ((Bindings::index == Index) || ...);
    }
 
    template <uint32_t Index>
    static constexpr vk::DescriptorType type_of() {
        vk::DescriptorType result = vk::DescriptorType::eSampler;
        ((Bindings::index == Index ? result = Bindings::type : result), ...);
        return result;
    }
 
private:
    static constexpr bool unique_indices() {
        constexpr std::array<uint32_t, sizeof...(Bindings)> indices{Bindings::index...};
        for (std::size_t i = 0; i < indices.size(); ++i)
            for (std::size_t j = i + 1; j < indices.size(); ++j)
                if (indices[i] == indices[j]) return false;
        return true;
    }
 
    vk::raii::DescriptorSetLayout layout = nullptr;
};
 
template <typename Layout>
class descriptor_set {
public:
    descriptor_set(const vk::raii::Device &device, const Layout &layout) : owner(&device) {
        const auto sizes = Layout::pool_sizes();
        vk::DescriptorPoolCreateInfo pool_info{};
        pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_info.maxSets = 1;
        pool_info.setPoolSizes(sizes);
        pool = vk::raii::DescriptorPool(device, pool_info);
 
        const vk::DescriptorSetLayout layout_handle = layout.handle();
        vk::DescriptorSetAllocateInfo alloc_info{};
        alloc_info.descriptorPool = *pool;
        alloc_info.setSetLayouts(layout_handle);
        vk::raii::DescriptorSets sets(device, alloc_info);
        set = std::move(sets.front());
    }
 
    template <uint32_t Index>
    void write_storage_image(vk::ImageView view, vk::ImageLayout image_layout) {
        static_assert(Layout::template has_binding<Index>(), "no binding with this index in the layout");
        static_assert(Layout::template type_of<Index>() == vk::DescriptorType::eStorageImage,
                      "binding is not a storage image");
 
        const vk::DescriptorImageInfo image_info{vk::Sampler{}, view, image_layout};
        vk::WriteDescriptorSet write{};
        write.dstSet = *set;
        write.dstBinding = Index;
        write.descriptorType = vk::DescriptorType::eStorageImage;
        write.setImageInfo(image_info);
        owner->updateDescriptorSets(write, {});
    }
 
    [[nodiscard]] vk::DescriptorSet handle() const { return *set; }
 
private:
    const vk::raii::Device *owner;
    vk::raii::DescriptorPool pool = nullptr;
    vk::raii::DescriptorSet set = nullptr;
};

} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_DESCRIPTORS_H
