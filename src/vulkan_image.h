#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImage
{
public:
    bool create(VulkanDevice& device, VkImageType image_type, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t array_layers,
                VkImageUsageFlags usage);
    void destroy();

    operator VkImage() const { return _image; }

    VkExtent3D get_extent() const { return _extent; }

    bool create_view(VkImageViewType viewType, VkImageAspectFlags aspect_mask, uint32_t base_mip_level, uint32_t level_count,
                     uint32_t base_array_layer, uint32_t layer_count, VkImageView& view);

private:
    VulkanDevice* _device = nullptr;
    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkExtent3D _extent = {};
    VkFormat _format = VK_FORMAT_UNDEFINED;
};
