#pragma once

#include "vulkan.h"

class Renderer
{
public:
    bool initialise(struct GLFWwindow* window);
    void shutdown();

    bool draw_frame();

private:
    bool create_instance();
    bool create_device();

    GLFWwindow* _window = nullptr;
    VkInstance _vulkan_instance = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VulkanPhysicalDevice _physical_device;
    VulkanDevice _device;
};