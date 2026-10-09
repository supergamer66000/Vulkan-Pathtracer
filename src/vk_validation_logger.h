#ifndef VULKAN_PATHTRACER_VK_VALIDATION_LOGGER_H
#define VULKAN_PATHTRACER_VK_VALIDATION_LOGGER_H

#include <iostream>
#include <format>
#include <string>
#include <string_view>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

namespace ansi {
    constexpr const char* reset  = "\033[0m";
    constexpr const char* bold   = "\033[1m";
    constexpr const char* dim    = "\033[2m";
    constexpr const char* red    = "\033[31m";
    constexpr const char* green  = "\033[32m";
    constexpr const char* yellow = "\033[33m";
    constexpr const char* cyan   = "\033[36m";
    constexpr const char* gray   = "\033[90m";
}

namespace vpt::vk_util {
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* data, void*) {

        // Severity -> label + color
        const char* color = ansi::gray;
        const char* label = "VERBOSE";
        switch (severity) {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:   color = ansi::red;    label = "ERROR";   break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: color = ansi::yellow; label = "WARNING"; break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:    color = ansi::green;  label = "INFO";    break;
            default: break;
        }

        // Message type tags
        std::string tags;
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)     tags += "[General]";
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)  tags += "[Validation]";
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) tags += "[Performance]";

        // Split "... The Vulkan spec states: <rule> (<url>)" from the main text
        std::string_view msg = data->pMessage ? data->pMessage : "";
        std::string_view spec, url;
        constexpr std::string_view spec_marker = "The Vulkan spec states:";
        if (const auto pos = msg.find(spec_marker); pos != std::string_view::npos) {
            spec = msg.substr(pos + spec_marker.size());
            msg  = msg.substr(0, pos);
            if (const auto u = spec.rfind(" (https"); u != std::string_view::npos) {
                url  = spec.substr(u + 2, spec.size() - u - 3); // strip " (" and ")"
                spec = spec.substr(0, u);
            }
        }
        while (!msg.empty()  && (msg.back()  == ' ' || msg.back()  == '\n')) msg.remove_suffix(1);
        while (!spec.empty() && spec.front() == ' ') spec.remove_prefix(1);

        std::string out = std::format("\n{}{}{} {}{}{} {}{}{}\n",
            ansi::bold, color, label, ansi::reset,
            ansi::cyan, tags, ansi::reset, "", "");
        out += std::format("  {}{}{}\n", color, msg, ansi::reset);

        if (data->pMessageIdName)
            out += std::format("  {}ID:{}   {}\n", ansi::dim, ansi::reset, data->pMessageIdName);

        if (!spec.empty())
            out += std::format("  {}Spec:{} {}\n", ansi::dim, ansi::reset, spec);
        if (!url.empty())
            out += std::format("  {}Link:{} {}{}{}\n", ansi::dim, ansi::reset, ansi::gray, url, ansi::reset);

        for (uint32_t i = 0; i < data->objectCount; ++i) {
            const auto& obj = data->pObjects[i];
            out += std::format("  {}Object {}:{} {} 0x{:x}{}\n",
                ansi::dim, i, ansi::reset,
                vk::to_string(obj.objectType), obj.objectHandle,
                obj.pObjectName ? std::format(" \"{}\"", obj.pObjectName) : "");
        }

        std::cerr << out;
        return vk::False;
    }
} // namespace vpt

#endif //VULKAN_PATHTRACER_VK_VALIDATION_LOGGER_H
