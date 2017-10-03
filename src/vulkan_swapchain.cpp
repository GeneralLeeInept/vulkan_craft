#include "vulkan_swapchain.h"

#include "vulkan.h"
#include "vulkan_device.h"

bool Swapchain::initialise(VulkanDevice& device)
{
    _device = &device;
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore((VkDevice)device, &create_info, nullptr, &_image_acquired_semaphore));
    return true;
}

void choose_surface_format(const std::vector<VkSurfaceFormatKHR>& surface_formats, VkFormat& format, VkColorSpaceKHR& colour_space)
{
    if ((surface_formats.size() == 1) && (surface_formats[0].format == VK_FORMAT_UNDEFINED))
    {
        format = VK_FORMAT_B8G8R8A8_UNORM;
        colour_space = surface_formats[0].colorSpace;
    }
    else
    {
        bool found_B8G8R8A8_UNORM = false;
        for (VkSurfaceFormatKHR surface_format : surface_formats)
        {
            if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                format = surface_format.format;
                colour_space = surface_format.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        if (!found_B8G8R8A8_UNORM)
        {
            format = surface_formats[0].format;
            colour_space = surface_formats[0].colorSpace;
        }
    }
}

bool Swapchain::create(uint32_t* width, uint32_t* height, bool vsync)
{
    VkSwapchainKHR old_swapchain = _swapchain;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK_RESULT(_device->get_surface_capabilities(surface_capabilities));

    if (surface_capabilities.currentExtent.width == UINT32_MAX)
    {
        _extent.width = *width;
        _extent.height = *height;
    }
    else
    {
        _extent = surface_capabilities.currentExtent;
        *width = surface_capabilities.currentExtent.width;
        *height = surface_capabilities.currentExtent.height;
    }

    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (VkPresentModeKHR present_mode : _device->get_present_modes())
    {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }

        if (!vsync && (swapchain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    uint32_t swapchain_image_count = surface_capabilities.minImageCount + 1;

    if ((surface_capabilities.maxImageCount > 0) && (swapchain_image_count > surface_capabilities.maxImageCount))
    {
        swapchain_image_count = surface_capabilities.maxImageCount;
    }

    VkColorSpaceKHR colour_space;
    choose_surface_format(_device->get_surface_formats(), _image_format, colour_space);

    VkSurfaceTransformFlagBitsKHR pretransform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        pretransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pretransform = surface_capabilities.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (VkCompositeAlphaFlagBitsKHR flag : composite_alpha_flags)
    {
        if (surface_capabilities.supportedCompositeAlpha & flag)
        {
            composite_alpha = flag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _device->get_surface();
    create_info.minImageCount = swapchain_image_count;
    create_info.imageFormat = _image_format;
    create_info.imageColorSpace = colour_space;
    create_info.imageExtent = _extent;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform = pretransform;
    create_info.imageArrayLayers = 1;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.presentMode = swapchain_present_mode;
    create_info.oldSwapchain = old_swapchain;
    create_info.clipped = VK_TRUE;
    create_info.compositeAlpha = composite_alpha;

    VK_CHECK_RESULT(vkCreateSwapchainKHR((VkDevice)*_device, &create_info, nullptr, &_swapchain));

    if (old_swapchain)
    {
        cleanup_swapchain(old_swapchain);
    }

    if (!get_images())
    {
        return false;
    }

    return true;
}

void Swapchain::destroy()
{
    if (_swapchain)
    {
        cleanup_swapchain(_swapchain);
    }

    if (_image_acquired_semaphore)
    {
        vkDestroySemaphore((VkDevice)*_device, _image_acquired_semaphore, nullptr);
    }
}

bool Swapchain::begin_frame()
{
    VK_CHECK_RESULT(
            vkAcquireNextImageKHR((VkDevice)*_device, _swapchain, UINT64_MAX, _image_acquired_semaphore, nullptr, &_acquired_image_index));
    return true;
}

bool Swapchain::end_frame(uint32_t wait_semaphore_count, VkSemaphore* wait_semaphores)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = wait_semaphore_count;
    present_info.pWaitSemaphores = wait_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;
    present_info.pImageIndices = &_acquired_image_index;
    VK_CHECK_RESULT(vkQueuePresentKHR(_device->get_graphics_queue(), &present_info));
    _acquired_image_index = UINT32_MAX;
    return true;
}

bool Swapchain::get_images()
{
    uint32_t image_count;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR((VkDevice)*_device, _swapchain, &image_count, nullptr));

    _images.resize(image_count);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR((VkDevice)*_device, _swapchain, &image_count, _images.data()));

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = _image_format;
    image_view_create_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    _image_views.resize(image_count);

    for (uint32_t i = 0; i < image_count; ++i)
    {
        image_view_create_info.image = _images[i];
        VK_CHECK_RESULT(vkCreateImageView((VkDevice)*_device, &image_view_create_info, nullptr, &_image_views[i]));
    }

    return true;
}

void Swapchain::cleanup_swapchain(VkSwapchainKHR swapchain)
{
    for (VkImageView& image_view : _image_views)
    {
        vkDestroyImageView((VkDevice)*_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR((VkDevice)*_device, swapchain, nullptr);
}
