#include "render_pass.h"

#include "vulkan.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

bool RenderPass::initialise(VulkanDevice& device, Swapchain& swapchain)
{
    _device = &device;
    _swapchain = &swapchain;
    return true;
}

void RenderPass::destroy()
{
    invalidate();
}

bool RenderPass::create()
{
    // Single pass forward-renderer
    VkAttachmentDescription colour_buffer = {};
    colour_buffer.format = _swapchain->get_image_format();
    colour_buffer.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_buffer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_buffer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_buffer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_buffer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_buffer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_buffer.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_buffer_reference = {};
    colour_buffer_reference.attachment = 0;
    colour_buffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_buffer_reference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &colour_buffer;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 0;
    create_info.pDependencies = nullptr;

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
