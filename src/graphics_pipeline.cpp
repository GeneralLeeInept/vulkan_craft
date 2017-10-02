#include "graphics_pipeline.h"

bool GraphicsPipeline::initialise(VulkanDevice& device, VkGraphicsPipelineCreateInfo& pipeline_create_info, VkPipelineLayoutCreateInfo& layout_create_info)
{
    _device = &device;

    _layout_create_info = layout_create_info;
    VULKAN_CHECK_RESULT(vkCreatePipelineLayout((VkDevice)*_device, &_layout_create_info, nullptr, &_layout));

    _shader_stages.resize(pipeline_create_info.stageCount);

    for (uint32_t stage = 0; stage < pipeline_create_info.stageCount; ++stage)
    {
        _shader_stages[stage] = pipeline_create_info.pStages[stage];
        // TODO: deep copy 
    }

    _vertex_input_state = *pipeline_create_info.pVertexInputState;  // TODO: deep copy
    _input_assembly_state = *pipeline_create_info.pInputAssemblyState;
    _rasterisation_state = *pipeline_create_info.pRasterizationState;
    _multisample_state = *pipeline_create_info.pMultisampleState;
    _depth_stencil_state = *pipeline_create_info.pDepthStencilState;

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
    _pipeline_create_info.pDepthStencilState = & _depth_stencil_state;
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

bool GraphicsPipeline::create(VulkanSwapchain& swapchain, VkRenderPass render_pass)
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain.get_extent().width;
    viewport.height = (float)swapchain.get_extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.get_extent();

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    _pipeline_create_info.pViewportState = &viewport_state;
    _pipeline_create_info.layout = _layout;
    _pipeline_create_info.renderPass = render_pass;

    VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines((VkDevice)*_device, nullptr, 1, &_pipeline_create_info, nullptr, &_pipeline));

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