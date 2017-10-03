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

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = _memory_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer((VkDevice)device, &buffer_create_info, nullptr, &_vertex_buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements((VkDevice)device, _vertex_buffer, &memory_requirements);

    const VkPhysicalDeviceMemoryProperties& memory_properties = device.get_memory_properties();
    uint32_t memory_type_index;
    VkMemoryPropertyFlags desired_properties =
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; ++memory_type_index)
    {
        if ((memory_requirements.memoryTypeBits & (1 << memory_type_index)) == 0)
        {
            continue;
        }

        if ((memory_properties.memoryTypes[memory_type_index].propertyFlags & desired_properties) == desired_properties)
        {
            break;
        }
    }

    if (memory_type_index == memory_properties.memoryTypeCount)
    {
        return false;
    }

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK_RESULT(vkAllocateMemory((VkDevice)device, &allocate_info, nullptr, &_device_memory));
    VK_CHECK_RESULT(vkBindBufferMemory((VkDevice)device, _vertex_buffer, _device_memory, 0));

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