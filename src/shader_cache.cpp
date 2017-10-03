#include "shader_cache.h"

#include <vector>

#include "file.h"

static VkShaderModule do_load(VkDevice device, const char* path)
{
    File fp;

    if (!fp.open(path, "rb"))
    {
        return VK_NULL_HANDLE;
    }

    uint32_t code_size = (fp.get_length() + 3) & ~3;
    std::vector<uint32_t> program(code_size / 4, 0);

    if (fp.read(program.data(), fp.get_length()) != fp.get_length())
    {
        return VK_NULL_HANDLE;
    }

    fp.close();

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.flags = 0;
    create_info.codeSize = code_size;
    create_info.pCode = program.data();

    VkShaderModule shader;

    if (VK_SUCCESS != vkCreateShaderModule(device, &create_info, nullptr, &shader))
    {
        return VK_NULL_HANDLE;
    }

    return shader;
}

bool ShaderCache::initialise(VkDevice device)
{
    _device = device;
    return true;
}

VkShaderModule ShaderCache::load(const char* path)
{
    std::map<std::string, VkShaderModule>::iterator it = _cache.find(path);
    VkShaderModule shader = VK_NULL_HANDLE;

    if (it != _cache.end())
    {
        shader = _cache[path];
    }
    else
    {
        shader = do_load(_device, path);
        _cache[path] = shader;
    }

    if (shader)
    {
        _ref_counts[shader]++;
    }

    return shader;
}

void ShaderCache::release(VkShaderModule shader)
{
    if (shader)
    {
        if (--_ref_counts[shader] == 0)
        {
            _ref_counts.erase(shader);

            for (std::map<std::string, VkShaderModule>::iterator it = _cache.begin(); it != _cache.end(); ++it)
            {
                if (it->second == shader)
                {
                    _cache.erase(it);
                    break;
                }
            }

            vkDestroyShaderModule(_device, shader, nullptr);
        }
    }
}
