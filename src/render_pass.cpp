#include "render_pass.h"

#include "vulkan.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

bool RenderPass::initialise(VulkanDevice& device, Swapchain& swapchain, DepthBuffer* depth_buffer)
{
    _device = &device;
    _swapchain = &swapchain;
    _depth_buffer = depth_buffer;
    return true;
}

void RenderPass::destroy()
{
    invalidate();
}

bool RenderPass::create()
{
    // Single pass forward-renderer
    VkAttachmentDescription attachments[2];

    VkAttachmentDescription& colour_buffer = attachments[0];
    colour_buffer = {};
    colour_buffer.format = _swapchain->get_image_format();
    colour_buffer.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_buffer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_buffer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_buffer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_buffer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_buffer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_buffer.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (_depth_buffer)
    {
        VkAttachmentDescription& depth_buffer = attachments[1];
        depth_buffer = {};
        depth_buffer.format = VK_FORMAT_D32_SFLOAT;//_depth_buffer->get_format();
        depth_buffer.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_buffer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_buffer.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_buffer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_buffer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_buffer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_buffer.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference colour_buffer_reference = {};
    colour_buffer_reference.attachment = 0;
    colour_buffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_buffer_reference = {};
    depth_buffer_reference.attachment = 1;
    depth_buffer_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_buffer_reference;
    subpass.pDepthStencilAttachment = _depth_buffer ? &depth_buffer_reference : nullptr;

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = _depth_buffer ? 2 : 1;
    create_info.pAttachments = attachments; 
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

    VK_CHECK_RESULT(vkCreateRenderPass((VkDevice)*_device, &create_info, nullptr, &_render_pass));

    return true;
}

void RenderPass::invalidate()
{
    if (_render_pass)
    {
        vkDestroyRenderPass((VkDevice)*_device, _render_pass, nullptr);
        _render_pass = VK_NULL_HANDLE;
    }
}
