#include "mesh_cache.h"

RenderMesh::RenderMesh(VkDevice device)
    : device(device)
{
}

RenderMesh::RenderMesh(RenderMesh&& other)
{
    device = other.device;
    index_buffer = other.index_buffer;
    vertex_buffer = other.vertex_buffer;
    index_count = other.index_count;
    memory = other.memory;
    _aabb = other._aabb;
    memset(&other, 0, sizeof(RenderMesh));
}

RenderMesh::~RenderMesh()
{
    if (index_buffer)
    {
        vkDestroyBuffer(device, index_buffer, nullptr);
    }

    if (vertex_buffer)
    {
        vkDestroyBuffer(device, vertex_buffer, nullptr);
    }

    if (memory)
    {
        vkFreeMemory(device, memory, nullptr);
    }
}
