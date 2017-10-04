#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanBuffer
{
public:
    bool create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);
    void destroy();

    bool map(void** mem);
    void unmap();

    VkDescriptorBufferInfo get_descriptor_info() const;

private:
    VulkanDevice* _device = nullptr;
    VkBuffer _buffer = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkDeviceSize _size = 0;
};