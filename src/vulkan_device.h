#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanPhysicalDevice
{
public:
    VulkanPhysicalDevice() = default;
    VulkanPhysicalDevice(VulkanPhysicalDevice&& rhs);

    VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&& rhs);

    void initialise(VkPhysicalDevice device);

    operator VkPhysicalDevice() const { return _device; }

    VkPhysicalDeviceProperties get_properties() const { return _properties; }
    VkPhysicalDeviceFeatures get_features() const { return _features; }
    uint32_t get_queue_family_index(VkQueueFlags flags, VkSurfaceKHR surface) const;

private:
    VkPhysicalDevice _device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties _properties = {};
    VkPhysicalDeviceFeatures _features = {};
    std::vector<VkQueueFamilyProperties> _queue_family_properties;
};

class VulkanDevice
{
public:
    bool create(VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface);
    void destroy();

    operator VkDevice() const { return _device; }

private:
    VulkanPhysicalDevice* _physical_device = nullptr;
    VkSurfaceKHR _surface;
    VkDevice _device;
};
