#include "depth_buffer.h"

#include "vulkan.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

bool DepthBuffer::initialise(VulkanDevice& device, Swapchain& swapchain)
{
    _device = &device;
    _swapchain = &swapchain;
    return true;
}

bool DepthBuffer::create()
{
    if (!_image.create(*_device, VK_IMAGE_TYPE_2D, VK_FORMAT_D32_SFLOAT, _swapchain->get_extent().width, _swapchain->get_extent().height, 1, 1,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
    {
        return false;
    }

    if (!_image.create_view(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1, _image_view))
    {
        return false;
    }

    return true;
}

void DepthBuffer::destroy()
{
    invalidate();
}

void DepthBuffer::invalidate()
{
    if (_image_view)
    {
        vkDestroyImageView((VkDevice)*_device, _image_view, nullptr);
        _image_view = VK_NULL_HANDLE;
    }

    _image.destroy();
}
