#include "vertex_buffer.h"

#include "vulkan.h"
#include "vulkan_device.h"

static uint32_t get_vertex_size(const VertexDecl& decl)
{
    uint32_t vertex_size = 0;

    for (const VertexElement& element : decl)
    {
        vertex_size += element.size;
    }

    return vertex_size;
}

bool VertexBuffer::create(VulkanDevice& device, const VertexDecl& decl, uint32_t count)
{
    _device = &device;
    _decl = decl;
    _count = count;
    _memory_size = get_vertex_size(decl) * count;

    if (!device.create_buffer(_memory_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              _vertex_buffer, _device_memory))
    {
        return false;
    }

    return true;
}

void VertexBuffer::destroy()
{
    if (_vertex_buffer)
    {
        vkDestroyBuffer((VkDevice)*_device, _vertex_buffer, nullptr);
        _vertex_buffer = VK_NULL_HANDLE;
    }

    if (_device_memory)
    {
        vkFreeMemory((VkDevice)*_device, _device_memory, nullptr);
        _device_memory = VK_NULL_HANDLE;
    }
}

bool VertexBuffer::map(void** mem)
{
    VK_CHECK_RESULT(vkMapMemory((VkDevice)*_device, _device_memory, 0, _memory_size, 0, mem));
    return true;
}

void VertexBuffer::unmap()
{
    vkUnmapMemory((VkDevice)*_device, _device_memory);
}

bool VertexBuffer::bind(std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes)
{
    uint32_t binding = (uint32_t)bindings.size();

    VkVertexInputBindingDescription binding_description;
    binding_description.binding = binding;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = get_vertex_size(_decl);
    bindings.push_back(binding_description);

    for (VertexElement& element : _decl)
    {
        VkVertexInputAttributeDescription attribute_description;
        attribute_description.binding = binding;
        attribute_description.format = element.format;
        attribute_description.location = element.location;
        attribute_description.offset = element.offset;
        attributes.push_back(attribute_description);
    }

    return true;
}