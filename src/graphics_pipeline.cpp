#include "graphics_pipeline.h"

#include "render_pass.h"
#include "vulkan.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

bool GraphicsPipeline::initialise(VulkanDevice& device, Swapchain& swapchain, RenderPass& render_pass,
                                  VkGraphicsPipelineCreateInfo& pipeline_create_info, VkPipelineLayoutCreateInfo& layout_create_info)
{
    _device = &device;
    _render_pass = &render_pass;
    _swapchain = &swapchain;

    _layout_create_info = layout_create_info;
    VK_CHECK_RESULT(vkCreatePipelineLayout((VkDevice)*_device, &_layout_create_info, nullptr, &_layout));

    _shader_stages.resize(pipeline_create_info.stageCount);
    memcpy(_shader_stages.data(), pipeline_create_info.pStages, sizeof(_shader_stages[0]) * pipeline_create_info.stageCount);

    _vertex_input_state = *pipeline_create_info.pVertexInputState;

    uint32_t binding_count = pipeline_create_info.pVertexInputState->vertexBindingDescriptionCount;

    if (binding_count)
    {
        const VkVertexInputBindingDescription* src_bindings = pipeline_create_info.pVertexInputState->pVertexBindingDescriptions;
        _vertex_bindings.resize(binding_count);
        memcpy(_vertex_bindings.data(), src_bindings, sizeof(_vertex_bindings[0]) * binding_count);
        _vertex_input_state.pVertexBindingDescriptions = _vertex_bindings.data();
    }

    uint32_t attribute_count = pipeline_create_info.pVertexInputState->vertexAttributeDescriptionCount; 

    if (attribute_count)
    {
        const VkVertexInputAttributeDescription* src_attributes = pipeline_create_info.pVertexInputState->pVertexAttributeDescriptions;
        _vertex_attributes.resize(attribute_count);
        memcpy(_vertex_attributes.data(), src_attributes, sizeof(_vertex_attributes[0]) * attribute_count);
        _vertex_input_state.pVertexAttributeDescriptions = _vertex_attributes.data();
    }

    // TODO: deep copy
    _input_assembly_state = *pipeline_create_info.pInputAssemblyState;
    _rasterisation_state = *pipeline_create_info.pRasterizationState;
    _multisample_state = *pipeline_create_info.pMultisampleState;
    _depth_stencil_state = *pipeline_create_info.pDepthStencilState;
    /////

    _attachments.resize(pipeline_create_info.pColorBlendState->attachmentCount);

    for (uint32_t attachment = 0; attachment < pipeline_create_info.pColorBlendState->attachmentCount; ++attachment)
    {
        _attachments[attachment] = pipeline_create_info.pColorBlendState->pAttachments[attachment];
    }

    _colour_blend_state = *pipeline_create_info.pColorBlendState;
    _colour_blend_state.pAttachments = _attachments.data();

    _pipeline_create_info = pipeline_create_info;
    _pipeline_create_info.pStages = _shader_stages.data();
    _pipeline_create_info.pVertexInputState = &_vertex_input_state;
    _pipeline_create_info.pInputAssemblyState = &_input_assembly_state;
    _pipeline_create_info.pRasterizationState = &_rasterisation_state;
    _pipeline_create_info.pMultisampleState = &_multisample_state;
    _pipeline_create_info.pDepthStencilState = &_depth_stencil_state;
    _pipeline_create_info.pColorBlendState = &_colour_blend_state;

    return true;
}

void GraphicsPipeline::destroy()
{
    invalidate();

    if (_layout)
    {
        vkDestroyPipelineLayout((VkDevice)*_device, _layout, nullptr);
        _layout = nullptr;
    }
}

bool GraphicsPipeline::create()
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_swapchain->get_extent().width;
    viewport.height = (float)_swapchain->get_extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain->get_extent();

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    _pipeline_create_info.pViewportState = &viewport_state;
    _pipeline_create_info.layout = _layout;
    _pipeline_create_info.renderPass = (VkRenderPass)*_render_pass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines((VkDevice)*_device, nullptr, 1, &_pipeline_create_info, nullptr, &_pipeline));

    return true;
}

void GraphicsPipeline::invalidate()
{
    if (_pipeline)
    {
        vkDestroyPipeline((VkDevice)*_device, _pipeline, nullptr);
        _pipeline = nullptr;
    }
}
