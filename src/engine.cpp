#include "engine.h"
#include "GLFW/glfw3.h"
#include "vk_util.h"
#include "vulkan/vulkan.hpp"

#include <iostream>
#include <chrono>
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
        this->init_vulkan_devices();
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

        vk_instance = vk_context.createInstance(instance_info);
 
        // Add validation layer. Connect it to the callback in vk_util.h
        vk::DebugUtilsMessengerCreateInfoEXT dbg{};
        dbg.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        dbg.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        dbg.pfnUserCallback = &vk_util::debug_callback;
        vk_debug = vk_instance.createDebugUtilsMessengerEXT(dbg);

        std::cout << "   "<< "---- Available Extensions ----" << std::endl;
        // Get the available vulkan extensions
        const auto extensions = vk_context.enumerateInstanceExtensionProperties();
        for (const auto& extension : extensions) {
            std::cout << "   " << extension.extensionName << '\n';
        }

    }

    void engine::init_vulkan_devices() {
        // Select a GPU
        const auto physicals_devices = vk_instance.enumeratePhysicalDevices();
        const auto selected_device = physicals_devices[2];
        for (const auto& gpu : physicals_devices) {
            const auto props = gpu.getProperties2().properties;
            std::cout << "GPU ID: " << props.deviceID << ", Name: " << props.deviceName << std::endl;
        }
        const auto physical_device_properties = selected_device.getProperties2();
        std::cout << std::endl;
        std::cout << "Selected GPU: " << physical_device_properties.properties.deviceName << std::endl;
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
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
