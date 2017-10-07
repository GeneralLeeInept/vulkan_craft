#include "texture_cache.h"

#include <Windows.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include "vulkan.h"
#include "vulkan_device.h"

class PngLoader
{
public:
    ~PngLoader() { reset(); }

    bool load(const char* path)
    {
        pixels = stbi_load(path, &width, &height, &components, STBI_rgb_alpha);
        return pixels != nullptr;
    }

    void reset()
    {
        if (pixels)
        {
            stbi_image_free(pixels);
        }

        width = 0;
        height = 0;
        components = 0;
    }

    VkDeviceSize get_size() { return width * height * components; }

    stbi_uc* pixels = nullptr;
    int width = 0;
    int height = 0;
    int components = 0;
};

bool Texture::create(VulkanDevice& device, const char* path)
{
    _device = &device;

    PngLoader loader;

    if (!loader.load(path))
    {
        return false;
    }

    if (!_staging_buffer.create(*_device, loader.get_size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        return false;
    }

    void* data;

    if (!_staging_buffer.map(&data))
    {
        return false;
    }

    memcpy(data, loader.pixels, loader.get_size());
    _staging_buffer.unmap();

    if (!_image.create(*_device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, loader.width, loader.height, 1, 1,
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT))
    {
        return false;
    }

    if (!_image.create_view(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, _image_view))
    {
        return false;
    }

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler((VkDevice)*_device, &sampler_info, nullptr, &_sampler));

    return true;
}

void Texture::destroy()
{
    if (_sampler)
    {
        vkDestroySampler((VkDevice)*_device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }

    if (_image_view)
    {
        vkDestroyImageView((VkDevice)*_device, _image_view, nullptr);
        _image_view = VK_NULL_HANDLE;
    }

    _image.destroy();
    _staging_buffer.destroy();
}

bool TextureArray::create(VulkanDevice& device, const std::vector<std::string>& paths)
{
    _device = &device;

    _layer_count = (uint32_t)paths.size();

    std::vector<PngLoader> loaders;
    loaders.resize(_layer_count);

    for (uint32_t layer = 0; layer < _layer_count; ++layer)
    {
        if (!loaders[layer].load(paths[layer].c_str()))
        {
            return false;
        }
    }

    for (uint32_t layer = 1; layer < _layer_count; ++layer)
    {
        if (loaders[layer].width != loaders[0].width || loaders[layer].height != loaders[0].height ||
            loaders[layer].components != loaders[0].components)
        {
            return false;
        }
    }

    VkDeviceSize buffer_size = 0;

    for (uint32_t layer = 0; layer < _layer_count; ++layer)
    {
        buffer_size += loaders[layer].get_size();
    }

    if (!_staging_buffer.create(*_device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        return false;
    }

    uint8_t* data;

    if (!_staging_buffer.map((void**)&data))
    {
        return false;
    }

    for (uint32_t layer = 0; layer < _layer_count; ++layer)
    {
        memcpy(data, loaders[layer].pixels, loaders[layer].get_size());
        data += loaders[layer].get_size();
    }

    _staging_buffer.unmap();

    if (!_image.create(*_device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, loaders[0].width, loaders[0].height, 1, _layer_count,
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT))
    {
        return false;
    }

    if (!_image.create_view(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, _layer_count, _image_view))
    {
        return false;
    }

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler((VkDevice)*_device, &sampler_info, nullptr, &_sampler));

    for (const std::string& path : paths)
    {
        char name[_MAX_FNAME];
        _splitpath(path.c_str(), nullptr, nullptr, name, nullptr);
        _layer_names.push_back(std::string(name));
    }

    return true;
}

bool TextureArray::create(VulkanDevice& device, const char* directory)
{
    std::vector<std::string> paths;
    std::string search_path = std::string(directory) + "/*.png";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = ::FindFirstFileA(search_path.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string path = std::string(directory) + "/" + std::string(fd.cFileName);
                paths.push_back(path);
            }
        } while (::FindNextFileA(hFind, &fd));

        ::FindClose(hFind);
    }

    if (paths.empty())
    {
        return false;
    }

    return create(device, paths);
}

void TextureArray::destroy()
{
    if (_sampler)
    {
        vkDestroySampler((VkDevice)*_device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }

    if (_image_view)
    {
        vkDestroyImageView((VkDevice)*_device, _image_view, nullptr);
        _image_view = VK_NULL_HANDLE;
    }

    _image.destroy();
    _staging_buffer.destroy();
}
