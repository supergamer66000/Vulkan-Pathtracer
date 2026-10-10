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
        vk_allocator = std::make_unique<vulkan::allocator>(*vk_instance, vk_device_ctx);
        this->init_vulkan_swapchain();
        this->init_vulkan_sync(); // Create the frames for the swapchain
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

     engine::GLFWExtensionSupportedInfo engine::get_glfw_extensions() {
        uint32_t glfw_extension_count = 0;
        auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        auto ext_prop = vk_context.enumerateInstanceExtensionProperties();
    
        for (uint32_t i = 0; i < glfw_extension_count; ++i) {
            if (std::ranges::none_of(ext_prop, [glfwExtension = glfw_ext[i]](auto const& extensionProperty) {
                return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
            }))
                throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_ext[i]));
        }
        return {glfw_extension_count, glfw_ext};
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
            .setPEnabledExtensionNames(required_device_extensions);

        vk_device_ctx.device = std::make_unique<vk::raii::Device>(*vk_device_ctx.physical_device, device_info);
        // vk_graphics_queue = std::make_unique<vk::raii::Queue>(vk_device_ctx.device.get()->getQueue(graphics_index, 0));

         // Create a graphics queue
        const uint32_t graphics_queue_index = 0;
        vk_graphics_queue = std::make_unique<vk::raii::Queue>(*vk_device_ctx.device, vk_graphics_queue_family_index, graphics_queue_index);

        vk::CommandPoolCreateInfo command_pool_info;
        command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer) // ???
            .setQueueFamilyIndex(vk_graphics_queue_family_index);

        vk_command_pool = std::make_unique<vk::raii::CommandPool>(vk_device_ctx.device->createCommandPool(command_pool_info));

        vk::CommandBufferAllocateInfo command_buffer_alloc_info; // Create command buffers for the device
        command_buffer_alloc_info.setCommandPool(**vk_command_pool);
        command_buffer_alloc_info.setCommandBufferCount(BUFFER_FRAME_COUNT); // Create a single buffer for now
        
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

    void engine::init_vulkan_sync() {
        const auto &device = *vk_device_ctx.device;
        frame_data.reserve(BUFFER_FRAME_COUNT);
        for (auto i = 0; i < BUFFER_FRAME_COUNT; ++i) {
            frame_data.push_back(FrameData{
                vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}),
                vk::raii::Fence(device, vk::FenceCreateInfo{
                    vk::FenceCreateFlagBits::eSignaled // Make it a signed fence
                })
            });
        }
        for (size_t i = 0; i < vk_swapchain->images().size(); ++i)
            vk_render_finished.emplace_back(device, vk::SemaphoreCreateInfo{});
    }

    void engine::draw_frame() {
        const auto &device = *vk_device_ctx.device;
        auto &frame = frame_data[current_frame];

        if (device.waitForFences(*frame.frames_in_flight, vk::True, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("Failed waiting for frame fence");

        uint32_t image_index = 0;
        try {
            const auto [result, index] = vk_swapchain->handle().acquireNextImage(UINT64_MAX, *frame.available_images);
            image_index = index;
        } catch (const vk::OutOfDateKHRError &) {
            return;
        }

        device.resetFences(*frame.frames_in_flight);

        const auto &cmd = (*vk_command_buffers)[current_frame];
        const vk::Image image = vk_swapchain->images()[image_index];

        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        vk_util::transition_image(cmd, image,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eAllTransfer, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eAllTransfer, vk::AccessFlagBits2::eTransferWrite);

        const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        const vk::ClearColorValue clear_color{std::array{0.1f, 0.2f, 0.4f, 1.0f}};
        cmd.clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, clear_color, range);

        vk_util::transition_image(cmd, image,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eAllTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone);

        cmd.end();

        const vk::SemaphoreSubmitInfo wait_info{*frame.available_images, 0, vk::PipelineStageFlagBits2::eAllTransfer};
        const vk::CommandBufferSubmitInfo cmd_info{*cmd};
        const vk::SemaphoreSubmitInfo signal_info{*vk_render_finished[image_index], 0, vk::PipelineStageFlagBits2::eAllCommands};

        vk::SubmitInfo2 submit{};
        submit.setWaitSemaphoreInfos(wait_info);
        submit.setCommandBufferInfos(cmd_info);
        submit.setSignalSemaphoreInfos(signal_info);
        vk_graphics_queue->submit2(submit, *frame.frames_in_flight);

        const vk::Semaphore wait_semaphore = *vk_render_finished[image_index];
        const vk::SwapchainKHR swapchain_handle = *vk_swapchain->handle();

        vk::PresentInfoKHR present_info{};
        present_info.setWaitSemaphores(wait_semaphore);
        present_info.setSwapchains(swapchain_handle);
        present_info.setImageIndices(image_index);

        try {
            const auto result = vk_graphics_queue->presentKHR(present_info);
        } catch (const vk::OutOfDateKHRError &) {}

        current_frame = (current_frame + 1) % BUFFER_FRAME_COUNT;
    }

    void engine::start() {
        auto end_time = std::chrono::high_resolution_clock::now();

        vk::FenceCreateInfo fence_info;

        uint32_t fps_counter;
        double fps_timer = 0;
        while (!glfwWindowShouldClose(window)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            double deltatime = std::chrono::duration<double>(start_time - end_time).count();
            end_time = std::chrono::high_resolution_clock::now();

            glfwPollEvents();
            this->draw_frame();
        }
        vk_device_ctx.device->waitIdle();
    }

    engine::~engine() {
        if (vk_device_ctx.device) vk_device_ctx.device->waitIdle();
        frame_data.clear();
        vk_render_finished.clear();
        vk_swapchain.reset();
        vk_surface = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
