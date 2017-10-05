#include "vulkan_image.h"

#include "vulkan.h"
#include "vulkan_device.h"

bool VulkanImage::create(VulkanDevice& device, VkImageType image_type, VkFormat format, uint32_t width, uint32_t height, uint32_t depth,
                         uint32_t array_layers, VkImageUsageFlags usage)
{
    _device = &device;

    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = image_type;
    create_info.format = format;
    create_info.extent = { width, height, depth };
    create_info.mipLevels = 1;
    ;
    create_info.arrayLayers = array_layers;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RESULT(vkCreateImage((VkDevice)*_device, &create_info, nullptr, &_image));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements((VkDevice)*_device, _image, &memory_requirements);
    VkDeviceSize offset;

    if (!_device->allocate_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements, _memory, offset))
    {
        return false;
    }

    VK_CHECK_RESULT(vkBindImageMemory((VkDevice)*_device, _image, _memory, offset));

    _extent = create_info.extent;
    _format = create_info.format;

    return true;
}

void VulkanImage::destroy()
{
    if (_image)
    {
        vkDestroyImage((VkDevice)*_device, _image, nullptr);
        _image = VK_NULL_HANDLE;
    }

    if (_memory)
    {
        vkFreeMemory((VkDevice)*_device, _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

bool VulkanImage::create_view(VkImageViewType viewType, VkImageAspectFlags aspect_mask, uint32_t base_mip_level, uint32_t level_count,
                              uint32_t base_array_layer, uint32_t layer_count, VkImageView& view)
{
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = _image;
    create_info.viewType = viewType;
    create_info.format = _format;
    create_info.subresourceRange.aspectMask = aspect_mask;
    create_info.subresourceRange.baseMipLevel = base_mip_level;
    create_info.subresourceRange.levelCount = level_count;
    create_info.subresourceRange.baseArrayLayer = base_array_layer;
    create_info.subresourceRange.layerCount = layer_count;
    VK_CHECK_RESULT(vkCreateImageView((VkDevice)*_device, &create_info, nullptr, &view));
    return true;
}