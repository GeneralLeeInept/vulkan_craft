#include "vulkan_device.h"

#include "texture_cache.h"
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

    if (!create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, _copy_command_pool))
    {
        return false;
    }

    return true;
}

void VulkanDevice::destroy()
{
    if (_copy_command_pool)
    {
        vkDestroyCommandPool(_device, _copy_command_pool, nullptr);
        _copy_command_pool = VK_NULL_HANDLE;
    }

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

bool VulkanDevice::create_command_pool(VkCommandPoolCreateFlags flags, VkCommandPool& command_pool)
{
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = flags;
    create_info.queueFamilyIndex = _graphics_queue_index;
    VK_CHECK_RESULT(vkCreateCommandPool(_device, &create_info, nullptr, &command_pool));
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

    VkDeviceSize offset;

    if (!allocate_memory(properties, memory_requirements, memory, offset))
    {
        return false;
    }

    VK_CHECK_RESULT(vkBindBufferMemory(_device, buffer, memory, offset));

    return true;
}

bool VulkanDevice::allocate_memory(VkMemoryPropertyFlags properties, VkMemoryRequirements requirements, VkDeviceMemory& memory, VkDeviceSize& offset)
{
    uint32_t memory_type_index;
    for (memory_type_index = 0; memory_type_index < _memory_properties.memoryTypeCount; ++memory_type_index)
    {
        if ((requirements.memoryTypeBits & (1 << memory_type_index)) == 0)
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
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = memory_type_index;
    VK_CHECK_RESULT(vkAllocateMemory(_device, &alloc_info, nullptr, &memory));

    offset = 0;

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

VkCommandBuffer VulkanDevice::begin_one_time_commands()
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = _copy_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers((VkDevice)_device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return command_buffer;
}

bool VulkanDevice::upload_texture(Texture& texture)
{
    VkCommandBuffer command_buffer = begin_one_time_commands();

    if (command_buffer == VK_NULL_HANDLE)
    {
        return false;
    }

    VkImageMemoryBarrier to_transition_dst = {};
    to_transition_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_transition_dst.srcAccessMask = 0;
    to_transition_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_transition_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_transition_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_transition_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transition_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transition_dst.image = texture._image;
    to_transition_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_transition_dst.subresourceRange.baseMipLevel = 0;
    to_transition_dst.subresourceRange.levelCount = 1;
    to_transition_dst.subresourceRange.baseArrayLayer = 0;
    to_transition_dst.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_transition_dst);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = texture._image.get_extent();
    vkCmdCopyBufferToImage(command_buffer, (VkBuffer)texture._staging_buffer, (VkImage)texture._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier to_shader = {};
    to_transition_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_transition_dst.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_transition_dst.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    to_transition_dst.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_transition_dst.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    to_transition_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transition_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transition_dst.image = texture._image;
    to_transition_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_transition_dst.subresourceRange.baseMipLevel = 0;
    to_transition_dst.subresourceRange.levelCount = 1;
    to_transition_dst.subresourceRange.baseArrayLayer = 0;
    to_transition_dst.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_transition_dst);

    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffer));

    submit(command_buffer, 0, nullptr, nullptr, 0, nullptr);

    return true;
}
