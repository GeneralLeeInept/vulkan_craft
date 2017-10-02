#pragma once

#include <vulkan/vulkan.h>

#include <map>
#include <string>

class ShaderCache
{
public:
    bool initialise(VkDevice device);

    VkShaderModule load(const char* path);
    void release(VkShaderModule shader);

private:
    std::map<std::string, VkShaderModule> _cache;
    std::map<VkShaderModule, int> _ref_counts;
    VkDevice _device;
};
