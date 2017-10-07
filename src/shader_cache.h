#pragma once

#include <vulkan/vulkan.h>

#include <map>
#include <string>

class ShaderCache
{
public:
    bool initialise(VkDevice device);

    VkShaderModule load(const std::wstring& path);
    void release(VkShaderModule shader);

private:
    struct CacheEntry
    {
        VkShaderModule shader_module;
        int ref_count;
    };

    typedef std::map<std::wstring, CacheEntry> Cache;
    Cache _cache;
    VkDevice _device;
};
