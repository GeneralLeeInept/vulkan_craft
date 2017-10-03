#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanDevice;

struct VertexElement
{
    uint32_t location; // layout location declared in the vertex shader
    VkFormat format;
    uint32_t offset;
    uint32_t size;
};

typedef std::vector<VertexElement> VertexDecl;

class VertexBuffer
{
public:
    bool create(VulkanDevice& device, const VertexDecl& decl, uint32_t count);
    void destroy();

    bool map(void** mem);
    void unmap();

    operator VkBuffer() { return _vertex_buffer; }

    bool bind(uint32_t binding, std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes);

private:
    VertexDecl _decl;
    uint32_t _count = 0;
    uint32_t _memory_size = 0;
    VkBuffer _vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _device_memory = VK_NULL_HANDLE;
    VulkanDevice* _device = nullptr;
};