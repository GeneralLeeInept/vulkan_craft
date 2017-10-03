#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;
class Swapchain;

class RenderPass
{
public:
    bool initialise(VulkanDevice& device, Swapchain& swapchain);
    void destroy();

    bool create();
    void invalidate();

    explicit operator VkRenderPass() { return _render_pass; }

private:
    VulkanDevice* _device;
    Swapchain* _swapchain;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
};