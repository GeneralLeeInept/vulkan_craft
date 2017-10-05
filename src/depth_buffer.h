#pragma once

#include "vulkan_image.h"

class Swapchain;
class VulkanDevice;

class DepthBuffer
{
public:
    bool initialise(VulkanDevice& device, Swapchain& swapchain);

    bool create();
    void destroy();

    void invalidate();

    VkImageView get_image_view() const { return _image_view; }

private:
    VulkanImage _image;
    VulkanDevice* _device = nullptr;
    Swapchain* _swapchain = nullptr;
    VkImageView _image_view = VK_NULL_HANDLE;
};
