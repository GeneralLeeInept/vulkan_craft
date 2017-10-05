#pragma once

#include <map>
#include <stdint.h>
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

class TextureCache
{
public:
    bool initialise();

private:
    bool load(const char* path);

    struct Texture
    {
        uint16_t width;
        uint16_t height;
        uint8_t channels;
        std::vector<uint8_t> pixels;
    };


    typedef std::map<const char*, Texture> Cache;
    Cache _textures;
};