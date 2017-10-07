#include "shader_cache.h"

#include <vector>

#include "file.h"

static VkShaderModule do_load(VkDevice device, const std::wstring& path)
{
    File fp;

    if (!fp.open(path))
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

VkShaderModule ShaderCache::load(const std::wstring& path)
{
    Cache::iterator it = _cache.find(path);
    VkShaderModule shader = VK_NULL_HANDLE;

    if (it != _cache.end())
    {
        shader = _cache[path].shader_module;
    }
    else
    {
        shader = do_load(_device, path);
        _cache[path].shader_module = shader;
    }

    if (shader)
    {
        _cache[path].ref_count++;
    }

    return shader;
}

void ShaderCache::release(VkShaderModule shader)
{
    if (shader)
    {
        for (Cache::value_type& cache_entry : _cache)
        {
            if (cache_entry.second.shader_module == shader)
            {
                if (--cache_entry.second.ref_count == 0)
                {
                    _cache.erase(cache_entry.first);
                    break;
                }
            }
        }

        vkDestroyShaderModule(_device, shader, nullptr);
    }
}
