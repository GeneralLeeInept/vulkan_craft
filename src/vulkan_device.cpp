#include "vulkan_device.h"

#include "vulkan.h"

VulkanDevice::VulkanDevice(VulkanDevice&& rhs)
{
    *this = std::move(rhs);
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& rhs)
{
    _queue_family_properties = std::move(rhs._queue_family_properties);
    _surface_formats = std::move(rhs._surface_formats);
    _present_modes = std::move(rhs._present_modes);

    _memory_properties = rhs._memory_properties;
    _properties = rhs._properties;
    _features = rhs._features;

    _physical_device = rhs._physical_device;
    _surface = rhs._surface;
    _device = rhs._device;
    _graphics_queue = rhs._graphics_queue;
    _graphics_queue_index = rhs._graphics_queue_index;

    rhs._memory_properties = {};
    rhs._properties = {};
    rhs._features = {};

    rhs._physical_device = VK_NULL_HANDLE;
    rhs._surface = VK_NULL_HANDLE;
    rhs._device = VK_NULL_HANDLE;
    rhs._graphics_queue = VK_NULL_HANDLE;
    rhs._graphics_queue_index = VK_NULL_HANDLE;

    return *this;
}

bool VulkanDevice::initialise(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceProperties(device, &_properties);
    vkGetPhysicalDeviceFeatures(device, &_features);

    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    _queue_family_properties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, _queue_family_properties.data());

    vkGetPhysicalDeviceMemoryProperties(device, &_memory_properties);

    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr));
    _surface_formats.resize(count);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, _surface_formats.data()));

    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr));
    _present_modes.resize(count);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, _present_modes.data()));

    _physical_device = device;
    _surface = surface;

    return true;
}

bool VulkanDevice::create()
{
    _graphics_queue_index = find_queue_family_index(VK_QUEUE_GRAPHICS_BIT);

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = _graphics_queue_index;
    queue_create_info.queueCount = 1;
    float queue_priorities = 0.0f;
    queue_create_info.pQueuePriorities = &queue_priorities;

    std::vector<const char*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
    create_info.ppEnabledExtensionNames = device_extensions.data();
    VK_CHECK_RESULT(vkCreateDevice(_physical_device, &create_info, nullptr, &_device));

    vkGetDeviceQueue(_device, queue_create_info.queueFamilyIndex, 0, &_graphics_queue);

    return true;
}

void VulkanDevice::destroy()
{
    if (_device)
    {
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }
}

VkResult VulkanDevice::get_surface_capabilities(VkSurfaceCapabilitiesKHR& surface_capabilities) const
{
    return vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities);
}

uint32_t VulkanDevice::find_queue_family_index(VkQueueFlags flags) const
{
    uint32_t valid = UINT32_MAX;

    for (uint32_t i = 0; i < _queue_family_properties.size(); ++i)
    {
        if (_queue_family_properties[i].queueCount == 0)
        {
            continue;
        }

        if (_queue_family_properties[i].queueFlags == flags)
        {
            VkBool32 surface_support = true;

            if ((flags & VK_QUEUE_GRAPHICS_BIT) && _surface)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, i, _surface, &surface_support);
            }

            if (surface_support)
            {
                return i;
            }
        }

        if (valid == UINT32_MAX && (_queue_family_properties[i].queueFlags & flags) == flags)
        {
            VkBool32 surface_support = true;

            if ((flags & VK_QUEUE_GRAPHICS_BIT) && _surface)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, i, _surface, &surface_support);
            }

            if (surface_support)
            {
                valid = i;
            }
        }
    }

    return valid;
}

bool VulkanDevice::create_command_pool(VkCommandPoolCreateFlags flags, VkCommandPool* command_pool)
{
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = flags;
    create_info.queueFamilyIndex = _graphics_queue_index;
    VK_CHECK_RESULT(vkCreateCommandPool(_device, &create_info, nullptr, command_pool));
    return true;
}

bool VulkanDevice::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                     VkDeviceMemory& memory)
{
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(_device, &create_info, nullptr, &buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memory_requirements);

    uint32_t memory_type_index;
    for (memory_type_index = 0; memory_type_index < _memory_properties.memoryTypeCount; ++memory_type_index)
    {
        if ((memory_requirements.memoryTypeBits & (1 << memory_type_index)) == 0)
        {
            continue;
        }

        if ((_memory_properties.memoryTypes[memory_type_index].propertyFlags & properties) == properties)
        {
            break;
        }
    }

    if (memory_type_index == _memory_properties.memoryTypeCount)
    {
        return false;
    }

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = memory_type_index;
    VK_CHECK_RESULT(vkAllocateMemory(_device, &alloc_info, nullptr, &memory));
    vkBindBufferMemory(_device, buffer, memory, 0);

    return true;
}

void VulkanDevice::submit(VkCommandBuffer buffer, uint32_t wait_semaphore_count, VkSemaphore* wait_semaphores,
                          const VkPipelineStageFlags* wait_stage_mask, uint32_t signal_semaphore_count, VkSemaphore* signal_semaphores)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    submit_info.signalSemaphoreCount = signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;
    vkQueueSubmit(_graphics_queue, 1, &submit_info, nullptr);
}
