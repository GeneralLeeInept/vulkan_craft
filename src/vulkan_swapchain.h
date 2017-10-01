#pragma once

#include <vulkan/vulkan.h>

class VulkanSwapchain
{
public:
    bool initialise(VulkanDevice& device);

    bool create(uint32_t *width, uint32_t *height, bool vsync);
    void destroy();

    VkFormat get_image_format() const { return _image_format; }

    bool begin_frame();
    bool end_frame(uint32_t wait_semaphore_count, VkSemaphore* wait_semaphores);

    VkExtent2D get_extent() const { return _extent; }
    const std::vector<VkImageView>& get_image_views() const { return _image_views; }
    VkSemaphore get_image_acquired_semaphore() const { return _image_acquired_semaphore; }
    VkImage get_acquired_image() const { return _acquired_image_index == UINT32_MAX ? VK_NULL_HANDLE : _images[_acquired_image_index]; }
    uint32_t get_acquired_image_index() const { return _acquired_image_index; }

private:
    bool get_images();
    void cleanup_swapchain(VkSwapchainKHR swapchain);

    std::vector<VkImage> _images;
    std::vector<VkImageView> _image_views;
    VkExtent2D _extent;
    VulkanDevice* _device = nullptr;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkSemaphore _image_acquired_semaphore = VK_NULL_HANDLE;
    VkFormat _image_format = VK_FORMAT_UNDEFINED;
    uint32_t _acquired_image_index = UINT32_MAX;
};