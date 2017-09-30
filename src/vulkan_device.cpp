#include "vulkan.h"

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&& rhs)
{
    *this = std::move(rhs);
}

VulkanPhysicalDevice& VulkanPhysicalDevice::operator=(VulkanPhysicalDevice&& rhs)
{
    _device = rhs._device;
    _properties = rhs._properties;
    _features = rhs._features;
    _queue_family_properties = std::move(rhs._queue_family_properties);

    rhs._device = VK_NULL_HANDLE;
    rhs._properties = {};
    rhs._features = {};

    return *this;
}

void VulkanPhysicalDevice::initialise(VkPhysicalDevice device)
{
    _device = device;

    vkGetPhysicalDeviceProperties(_device, &_properties);

    vkGetPhysicalDeviceFeatures(_device, &_features);

    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &count, nullptr);
    _queue_family_properties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &count, _queue_family_properties.data());
}

uint32_t VulkanPhysicalDevice::get_queue_family_index(VkQueueFlags flags, VkSurfaceKHR surface) const
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

            if (surface)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface, &surface_support);
            }

            if (surface_support)
            {
                return i;
            }
        }

        if (valid == UINT32_MAX && (_queue_family_properties[i].queueFlags & flags) == flags)
        {
            VkBool32 surface_support = true;

            if (surface)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface, &surface_support);
            }

            if (surface_support)
            {
                valid = i;
            }
        }
    }

    return valid;
}

bool VulkanDevice::create(VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface)
{
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = physical_device.get_queue_family_index(VK_QUEUE_GRAPHICS_BIT, surface);
    queue_create_info.queueCount = 1;
    float queue_priorities = 0.0f;
    queue_create_info.pQueuePriorities = &queue_priorities;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    VULKAN_CHECK_RESULT(vkCreateDevice(physical_device, &create_info, nullptr, &_device));

    _physical_device = &physical_device;
    _surface = surface;

    return true;
}

void VulkanDevice::destroy()
{
    if (_device)
    {
        vkDestroyDevice(_device, nullptr);
    }
}

