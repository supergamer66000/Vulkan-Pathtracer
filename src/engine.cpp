#include "engine.h"
#include "GLFW/glfw3.h"
#include "vk_swapchain.h"
#include "vk_util.h"
#include "vk_validation_logger.h"
#include "vulkan/vulkan.hpp"

#include <glm/glm.hpp>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vpt {
    engine::engine() : 
        width(1280), height(720)
    {
       // Init GLFW //
        if (!glfwInit())
            throw std::runtime_error("Error initializing glfw");
       
        this->init_glfw();
        this->init_vulkan_instance();

        // Create the surface
        auto maybe_surface = this->create_surface(window, *vk_instance);
        if (!maybe_surface) throw std::runtime_error("Error creating the glfw surface for renderering.");
        vk_surface = std::move(maybe_surface.value());

        this->init_vulkan_physical_devices();
        this->init_vulkan_device();
    }

    void engine::init_glfw() {
        // glfw Hints
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE , false);

        window = glfwCreateWindow(width, height, WINDOW_NAME, nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Error creating glfw window");
        glfwMakeContextCurrent(window);
    }

    void engine::init_vulkan_instance() {
        /*
         *  Create the app info the the vulkan renderer.
         */
        vk::ApplicationInfo app_info;
        app_info.pApplicationName = WINDOW_NAME;
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = vk::ApiVersion14; // Vulkan 1.4

        // Instance Info //
        auto required_instance_extensions = vk_util::get_required_instance_extension_by_glfw();
        required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // enabled_validation_layers
        const auto instance_extension_properties = vk_context.enumerateInstanceExtensionProperties();
        const auto unsupported_instance_properties = vk_util::find_unsupported(
            required_instance_extensions, instance_extension_properties,
            [] (const auto& extension_property) { return extension_property.extensionName; });
        if (unsupported_instance_properties != required_instance_extensions.end())
            throw std::runtime_error("Required extension not supported" + std::string(*unsupported_instance_properties));

        // Validation Layers //
        std::vector<const char*> required_layers;
        if (use_validation_layers)
            required_layers.assign(enabled_validation_layers.begin(), enabled_validation_layers.end());

        // Get the required extension
        const auto required_layer_properties = vk_context.enumerateInstanceLayerProperties();
        const auto unsupported_layers = vpt::vk_util::find_unsupported(required_layers, required_layer_properties,
            [] (const auto& property) { return property.layerName; }
        );

        if (unsupported_layers != required_layers.end())
            throw std::runtime_error("Required layers not supported: " + std::string(*unsupported_layers));

        vk::InstanceCreateInfo instance_info;
        instance_info.setPApplicationInfo(&app_info)
            .setPEnabledLayerNames(required_layers)
            .setPEnabledExtensionNames(required_instance_extensions);

        vk_instance = std::make_unique<vk::raii::Instance>(vk_context.createInstance(instance_info));
 
        // Add validation layer. Connect it to the callback in vk_util.h
        vk::DebugUtilsMessengerCreateInfoEXT dbg{};
        dbg.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        dbg.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        dbg.pfnUserCallback = &vk_util::debug_callback;
        vk_debug = vk_instance->createDebugUtilsMessengerEXT(dbg);

        std::cout << "   "<< "---- Available Extensions ----" << std::endl;
        // Get the available vulkan extensions
        const auto extensions = vk_context.enumerateInstanceExtensionProperties();
        for (const auto& extension : extensions) {
            std::cout << "   " << extension.extensionName << '\n';
        }
    }

    void engine::init_vulkan_physical_devices() {
        // Select a GPU
        const auto physicals_devices = vk_instance->enumeratePhysicalDevices();
        vk_device_ctx.physical_device = std::make_unique<vk::raii::PhysicalDevice>(physicals_devices.front());

        for (const auto& gpu : physicals_devices) {
            const auto props = gpu.getProperties2().properties;
            std::cout << "GPU ID: " << props.deviceID << ", Name: " << props.deviceName << std::endl;
        }
        const auto physical_device_properties = vk_device_ctx.physical_device->getProperties2();
    
        std::cout << std::endl;
        std::cout << "Selected GPU: " << physical_device_properties.properties.deviceName << std::endl;
    }

    void engine::init_vulkan_device() {
        const auto graphics_family_queue_properties = vk_device_ctx.physical_device->getQueueFamilyProperties2();
        std::optional<uint32_t> found_family;
        for (uint32_t i = 0; i < graphics_family_queue_properties.size(); ++i) {
            const bool graphics = static_cast<bool>(
                graphics_family_queue_properties[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics);
            if (graphics && vk_device_ctx.physical_device->getSurfaceSupportKHR(i, *vk_surface)) {
                found_family = i;
                break;
            }
        }
        if (!found_family) throw std::runtime_error("Failed to find graphics family for device");
        vk_graphics_queue_family_index = *found_family;

        // Create the device Queue
        constexpr float queue_priority = 0.5f;
        vk::DeviceQueueCreateInfo device_queue_create_info;
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.queueFamilyIndex = vk_graphics_queue_family_index;
        device_queue_create_info.pQueuePriorities = &queue_priority;

        // Device Features
        const auto available_physical_device_features = vk_device_ctx.physical_device->getFeatures2();
        const auto physical_physical_device_properities = vk_device_ctx.physical_device->enumerateDeviceExtensionProperties();
        const auto unsupported_physical_device_properities = vk_util::find_unsupported(required_device_extensions, physical_physical_device_properities, 
            [] (const auto& property) {
                return property.extensionName;
            });
        if (unsupported_physical_device_properities != required_device_extensions.end())
            throw std::runtime_error("Required layers not supported: " + std::string(*unsupported_physical_device_properities));

        // Enable device features
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDeviceVulkan13Features
            // vk::PhysicalDeviceVulkan14Features
        > device_features{};
        device_features.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress = true;
        device_features.get<vk::PhysicalDeviceVulkan12Features>().descriptorIndexing = true;
        device_features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
        device_features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;

        vk::DeviceCreateInfo device_info;
        device_info.setPNext(&device_features)
            .setQueueCreateInfos(device_queue_create_info)
            .setPpEnabledExtensionNames(required_device_extensions.data());

        vk_device_ctx.device = std::make_unique<vk::raii::Device>(*vk_device_ctx.physical_device, device_info);
        // vk_graphics_queue = std::make_unique<vk::raii::Queue>(vk_device_ctx.device.get()->getQueue(graphics_index, 0));

        vk::CommandPoolCreateInfo command_pool_info;
        command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer) // ???
            .setQueueFamilyIndex(vk_graphics_queue_family_index);

        vk_command_pool = std::make_unique<vk::raii::CommandPool>(vk_device_ctx.device->createCommandPool(command_pool_info));

        vk::CommandBufferAllocateInfo command_buffer_alloc_info; // Create command buffers for the device
        command_buffer_alloc_info.setCommandPool(**vk_command_pool);
        command_buffer_alloc_info.setCommandBufferCount(1); // Create a single buffer for now
        
        vk_command_buffers = std::make_unique<vk::raii::CommandBuffers>(*vk_device_ctx.device, command_buffer_alloc_info);
    }

    void engine::init_vulkan_swapchain() {
        glm::ivec2 frame_buffer_size;
        glfwGetFramebufferSize(window, &frame_buffer_size.x, &frame_buffer_size.x);

        vk_swapchain = std::make_unique<vulkan::swapchain>(
            vk_device_ctx, vk_surface,
            vk::Extent2D{static_cast<uint32_t>(frame_buffer_size.x), static_cast<uint32_t>(frame_buffer_size.y)},
            vk_graphics_queue_family_index
        );
    }

    void engine::start() {
        auto end_time = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            double deltatime = std::chrono::duration<double>(end_time - start_time).count();
            end_time = std::chrono::high_resolution_clock::now();

            glfwPollEvents();
        }
    }

    engine::~engine() {
      
        glfwTerminate();
    }
};
