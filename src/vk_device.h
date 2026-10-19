#ifndef VULKAN_PATHTRACER_VK_DEVICE_H
#define VULKAN_PATHTRACER_VK_DEVICE_H

#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "vk_util.h"

namespace vpt::vulkan {

using ENABLED_VULKAN_FEATURES = vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceVulkan14Features
>;

// TODO make a design that allows for easy queue creation and passing in BUFFER_FRAME_COUNT
// TODO make the same design for command buffers and command pools
class vk_device {
public:
    vk_device(const vk::raii::Instance &instance, const vk::raii::SurfaceKHR &surface, const uint32_t BUFFER_FRAME_COUNT) {
        const auto physicals_devices = instance.enumeratePhysicalDevices();
        physical_device = physicals_devices.front();

        const auto queue_info = create_queue(physical_device, surface);
        const auto family_queue_index = queue_info.queueFamilyIndex;

        const auto available_physical_device_features = physical_device.getFeatures2();
        const auto physical_physical_device_properities = physical_device.enumerateDeviceExtensionProperties();
        const auto unsupported_physical_device_properities = vk_util::find_unsupported(required_device_extensions, physical_physical_device_properities, 
            [] (const auto& property) { return property.extensionName; });
        if (unsupported_physical_device_properities != required_device_extensions.end())
            throw std::runtime_error("Required layers not supported: " + std::string(*unsupported_physical_device_properities));

        vk::DeviceCreateInfo device_info{};
        device_info
            .setPNext(&features)
            .setQueueCreateInfos(queue_info)
            .setPEnabledExtensionNames(required_device_extensions);
        device = vk::raii::Device(physical_device, device_info);

        const uint32_t queue_index = 0;
        queue = vk::raii::Queue(device, family_queue_index, queue_index);

        vk::CommandPoolCreateInfo command_pool_info;
        command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer) // ???
            .setQueueFamilyIndex(family_queue_index);

        command_pool = vk::raii::CommandPool(device.createCommandPool(command_pool_info));

        vk::CommandBufferAllocateInfo command_buffer_alloc_info; // Create command buffers for the device
        command_buffer_alloc_info.setCommandPool(*command_pool);
        command_buffer_alloc_info.setCommandBufferCount(BUFFER_FRAME_COUNT); // Create a single buffer for now

        command_buffers = vk::raii::CommandBuffers(device, command_buffer_alloc_info);
    }
    vk_device();

    [[nodiscard]] const vk::raii::Device&         get_device()          { return device; }
    [[nodiscard]] const vk::raii::PhysicalDevice& get_physical_device() { return physical_device; }
    [[nodiscard]] const vk::raii::CommandBuffers&  get_command_buffer()  { return command_buffers; }
private:
    static const ENABLED_VULKAN_FEATURES enabled_features() {
        ENABLED_VULKAN_FEATURES feature_chain;
        feature_chain.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress = true;
        feature_chain.get<vk::PhysicalDeviceVulkan12Features>().descriptorIndexing  = true;
        feature_chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering    = true;
        feature_chain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2    = true;
        feature_chain.get<vk::PhysicalDeviceVulkan14Features>().maintenance5        = true;
        return feature_chain;
    }
    const ENABLED_VULKAN_FEATURES features = enabled_features();
    const std::vector<const char*> required_device_extensions = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSynchronization2ExtensionName
    };

    void create_physical_device() {
    }

    // Just creates a queue for the graphics family
    // TODO create a way to create queues with templates
    static const vk::DeviceQueueCreateInfo create_queue(
        const vk::raii::PhysicalDevice &physical_device,
        const vk::raii::SurfaceKHR &surface
    ) { // TEMP
        const auto graphics_family_queue_properties = physical_device.getQueueFamilyProperties2();
        std::optional<uint32_t> found_family;
        for (uint32_t i = 0; i < graphics_family_queue_properties.size(); ++i) {
            const bool graphics = static_cast<bool>(
                graphics_family_queue_properties[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics);
            if (graphics && physical_device.getSurfaceSupportKHR(i, *surface)) {
                found_family = i;
                break;
            }
        }
        if (!found_family) throw std::runtime_error("Failed to find graphics family for device");
        const auto vk_graphics_queue_family_index = *found_family;

        // Create the device Queue
        constexpr float queue_priority = 0.5f;
        vk::DeviceQueueCreateInfo device_queue_create_info;
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.queueFamilyIndex = vk_graphics_queue_family_index;
        device_queue_create_info.pQueuePriorities = &queue_priority;
        return device_queue_create_info;
    }

    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;


    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::CommandBuffers command_buffers = nullptr;
};

} // namespace vpt::vulkan

#endif //VULKAN_PATHTRACER_VK_DEVICE_H
