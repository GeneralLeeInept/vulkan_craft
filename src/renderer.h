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
    void invalidate();

    bool create_instance();
    bool create_device();
    bool create_semaphores();
    bool create_command_pool();
    bool create_render_pass();
    bool create_frame_buffers();

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    std::vector<VkFramebuffer> _frame_buffers;
    GLFWwindow* _window = nullptr;
    VkInstance _vulkan_instance = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
    VkCommandPool _command_pool = VK_NULL_HANDLE;
    VkSemaphore _drawing_complete_semaphore = VK_NULL_HANDLE;

    bool _valid_state = false;
};