#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanBuffer
{
public:
    ~VulkanBuffer() { destroy(); }

    bool create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);
    void destroy();

    operator VkBuffer() const { return _buffer; }

    bool map(void** mem);
    void unmap();

    VkDescriptorBufferInfo get_descriptor_info() const;
    VkDeviceSize get_device_size() const { return _size; }

private:
    VulkanDevice* _device = nullptr;
    VkBuffer _buffer = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkDeviceSize _size = 0;
};