#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Texture;

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

    const VkPhysicalDeviceMemoryProperties& get_memory_properties() const { return _memory_properties; }
    const VkPhysicalDeviceProperties& get_properties() const { return _properties; }
    const VkPhysicalDeviceFeatures& get_features() const { return _features; }
    const VkSurfaceKHR& get_surface() const { return _surface; }
    VkResult get_surface_capabilities(VkSurfaceCapabilitiesKHR& surface_capabilities) const;
    const VkQueue& get_graphics_queue() const { return _graphics_queue; }
    uint32_t get_graphics_queue_index() const { return _graphics_queue_index; }

    const std::vector<VkSurfaceFormatKHR>& get_surface_formats() const { return _surface_formats; }
    const std::vector<VkPresentModeKHR>& get_present_modes() const { return _present_modes; }

    uint32_t find_queue_family_index(VkQueueFlags flags) const;

    bool create_command_pool(VkCommandPoolCreateFlags flags, VkCommandPool& command_pool);
    bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);

    bool allocate_memory(VkMemoryPropertyFlags properties, VkMemoryRequirements requirements, VkDeviceMemory& memory, VkDeviceSize& offset);

    void submit(VkCommandBuffer buffer, uint32_t wait_semaphore_count, VkSemaphore* wait_semaphores, const VkPipelineStageFlags* wait_stage_mask,
                uint32_t signal_semaphore_count, VkSemaphore* signal_semaphores);

    VkCommandBuffer begin_one_time_commands();
    bool upload_texture(Texture& texture);

private:
    std::vector<VkQueueFamilyProperties> _queue_family_properties;
    std::vector<VkSurfaceFormatKHR> _surface_formats;
    std::vector<VkPresentModeKHR> _present_modes;
    VkPhysicalDeviceMemoryProperties _memory_properties = {};
    VkPhysicalDeviceProperties _properties = {};
    VkPhysicalDeviceFeatures _features = {};
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphics_queue = VK_NULL_HANDLE;
    uint32_t _graphics_queue_index = UINT32_MAX;
    VkCommandPool _copy_command_pool = VK_NULL_HANDLE;
};
