#include "vulkan_buffer.h"

#include "vulkan.h"
#include "vulkan_device.h"

bool VulkanBuffer::create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties)
{
    _device = &device;
    _size = size;
    return device.create_buffer(size, usage, memory_properties, _buffer, _memory);
}

void VulkanBuffer::destroy()
{
    if (_buffer)
    {
        vkDestroyBuffer((VkDevice)*_device, _buffer, nullptr);
        _buffer = VK_NULL_HANDLE;
    }

    if (_memory)
    {
        vkFreeMemory((VkDevice)*_device, _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

bool VulkanBuffer::map(void** mem)
{
    VK_CHECK_RESULT(vkMapMemory((VkDevice)*_device, _memory, 0, _size, 0, mem));
    return true;
}

void VulkanBuffer::unmap()
{
    vkUnmapMemory((VkDevice)*_device, _memory);
}

VkDescriptorBufferInfo VulkanBuffer::get_descriptor_info() const
{
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = _buffer;
    buffer_info.offset = 0;
    buffer_info.range = _size;
    return buffer_info;
}
