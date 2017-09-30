#pragma once

#include <vulkan/vulkan.h>

class VulkanSwapchain
{
public:
    void initialise(VulkanDevice& device);

    bool create(uint32_t *width, uint32_t *height, bool vsync);
    void destroy();

private:
    bool get_images(VkFormat image_format);
    void cleanup_swapchain(VkSwapchainKHR swapchain);

    VulkanDevice* _device = nullptr;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _images;
    std::vector<VkImageView> _image_views;
};