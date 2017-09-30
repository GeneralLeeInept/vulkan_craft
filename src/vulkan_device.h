#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanDevice
{
public:
    VulkanDevice() = default;
    VulkanDevice(VulkanDevice&& rhs);

    VulkanDevice& operator=(VulkanDevice&& rhs);

    bool initialise(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool create();
    void destroy();

    explicit operator VkPhysicalDevice() const { return _physical_device; }
    explicit operator VkDevice() const { return _device; }

    VkPhysicalDeviceProperties get_properties() const { return _properties; }
    VkPhysicalDeviceFeatures get_features() const { return _features; }
    VkSurfaceKHR get_surface() const { return _surface; }
    VkSurfaceCapabilitiesKHR get_surface_capabilities() const { return _surface_capabilities; }

    const std::vector<VkSurfaceFormatKHR>& get_surface_formats() const { return _surface_formats; }
    const std::vector<VkPresentModeKHR>& get_present_modes() const { return _present_modes; }

    uint32_t find_queue_family_index(VkQueueFlags flags) const;

private:
    std::vector<VkQueueFamilyProperties> _queue_family_properties;
    std::vector<VkSurfaceFormatKHR> _surface_formats;
    std::vector<VkPresentModeKHR> _present_modes;
    VkPhysicalDeviceProperties _properties = {};
    VkPhysicalDeviceFeatures _features = {};
    VkSurfaceCapabilitiesKHR _surface_capabilities = {};
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphics_queue = VK_NULL_HANDLE;
    uint32_t _graphics_queue_index = UINT32_MAX;
};
