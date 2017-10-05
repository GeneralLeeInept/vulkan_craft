#pragma once

#include <vulkan/vulkan.h>

class DepthBuffer;
class Swapchain;
class VulkanDevice;

class RenderPass
{
public:
    bool initialise(VulkanDevice& device, Swapchain& swapchain, DepthBuffer* depth_buffer);
    void destroy();

    bool create();
    void invalidate();

    explicit operator VkRenderPass() { return _render_pass; }

private:
    VulkanDevice* _device = nullptr;
    Swapchain* _swapchain = nullptr;
    DepthBuffer* _depth_buffer = nullptr;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
};