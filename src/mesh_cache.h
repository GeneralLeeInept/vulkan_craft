#pragma once

#include <vulkan/vulkan.h>

#include "geometry.h"
#include "world.h"

class RenderMesh
{
public:
    RenderMesh(VkDevice device);
    RenderMesh(RenderMesh&& other);
    ~RenderMesh();

    VkDevice device = VK_NULL_HANDLE;
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    uint32_t index_count = 0;

    geometry::aabb _aabb;
};

class MeshCache
{
};