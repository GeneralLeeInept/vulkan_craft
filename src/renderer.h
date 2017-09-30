#pragma once

#include "vulkan.h"

class Renderer
{
public:
    bool initialise(struct GLFWwindow* window);
    void shutdown();

    bool set_window_size(uint32_t width, uint32_t height);

    bool draw_frame();

private:
    bool create_instance();
    bool create_device();

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    GLFWwindow* _window = nullptr;
    VkInstance _vulkan_instance = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
};