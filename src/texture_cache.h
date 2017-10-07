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
    bool create(VulkanDevice& device, const std::wstring& path);
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
    bool create(VulkanDevice& device, const std::vector<std::wstring>& paths);
    bool create(VulkanDevice& device, const std::wstring& directory);
    void destroy();

    int layer_index(const std::wstring& name);

public:
    VulkanBuffer _staging_buffer;
    VulkanImage _image;
    VulkanDevice* _device = nullptr;
    VkImageView _image_view = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    uint32_t _layer_count = 0;
    std::vector<std::wstring> _layer_names;
};
