#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vulkan_buffer.h"
#include "vulkan_image.h"

class VulkanDevice;

class Texture
{
public:
    bool create(VulkanDevice& device, const char* path);
    void destroy();

public:
    VulkanBuffer _staging_buffer;
    VulkanImage _image;
    VulkanDevice* _device = nullptr;
    VkImageView _image_view = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
};

class TextureArray
{
public:
    bool create(VulkanDevice& device, const std::vector<std::string>& paths);
    bool create(VulkanDevice& device, const char* directory);
    void destroy();

    int layer_index(const std::string& name);

public:
    VulkanBuffer _staging_buffer;
    VulkanImage _image;
    VulkanDevice* _device = nullptr;
    VkImageView _image_view = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    uint32_t _layer_count = 0;
    std::vector<std::string> _layer_names;
};
