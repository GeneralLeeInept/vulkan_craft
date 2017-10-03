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

    VK_CHECK_RESULT(vkCreatePipelineLayout((VkDevice)*_device, &layout_create_info, nullptr, &_layout));

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

void GraphicsPipelineFactory::initialise(VulkanDevice& device, Swapchain& swapchain, RenderPass& render_pass, uint32_t subpass)
{
    _device = &device;
    _swapchain = &swapchain;
    _render_pass = &render_pass;
    _subpass = subpass;
}

void GraphicsPipelineFactory::set_shader(VkShaderStageFlagBits stage, VkShaderModule shader, const char* name)
{
    if (shader && name)
    {
        _shader_stages[stage] = std::make_pair(shader, name);
    }
    else
    {
        _shader_stages.erase(stage);
    }
}

void GraphicsPipelineFactory::set_input_assembly_state(VkPrimitiveTopology topology, bool primtive_restart)
{
    _primitive_topology = topology;
    _primitive_restart = primtive_restart;
}

static uint32_t get_vertex_size(const VertexDecl& decl)
{
    uint32_t vertex_size = 0;

    for (const VertexElement& element : decl)
    {
        vertex_size += element.size;
    }

    return vertex_size;
}

static void create_input_state(const VertexDecl& vertex_decl, std::vector<VkVertexInputBindingDescription>& bindings,
                               std::vector<VkVertexInputAttributeDescription>& attributes)
{
    uint32_t binding = (uint32_t)bindings.size();

    VkVertexInputBindingDescription binding_description;
    binding_description.binding = binding;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = get_vertex_size(vertex_decl);
    bindings.push_back(binding_description);

    for (const VertexElement& element : vertex_decl)
    {
        VkVertexInputAttributeDescription attribute_description;
        attribute_description.binding = binding;
        attribute_description.format = element.format;
        attribute_description.location = element.location;
        attribute_description.offset = element.offset;
        attributes.push_back(attribute_description);
    }
}

void GraphicsPipelineFactory::set_vertex_decl(const VertexDecl& vertex_decl)
{
    create_input_state(vertex_decl, _vertex_bindings, _vertex_attributes);
}

void GraphicsPipelineFactory::add_descriptor_set_layout(VkDescriptorSetLayout layout)
{
    _descriptor_set_layouts.push_back(layout);
}

bool GraphicsPipelineFactory::create_pipeline(GraphicsPipeline& pipeline)
{
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

    for (const std::pair<VkShaderStageFlagBits, ShaderRef>& shader_stage : _shader_stages)
    {
        VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
        shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_info.stage = shader_stage.first;
        shader_stage_create_info.module = shader_stage.second.first;
        shader_stage_create_info.pName = shader_stage.second.second;
        shader_stages.push_back(shader_stage_create_info);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = (uint32_t)_vertex_bindings.size();
    vertex_input_state_create_info.pVertexBindingDescriptions = _vertex_bindings.data();
    vertex_input_state_create_info.vertexAttributeDescriptionCount = (uint32_t)_vertex_attributes.size();
    vertex_input_state_create_info.pVertexAttributeDescriptions = _vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = _primitive_topology;
    input_assembly_state_create_info.primitiveRestartEnable = _primitive_restart ? VK_TRUE : VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellation_state_create_info = {};
    tessellation_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    VkPipelineColorBlendAttachmentState colour_blend_attachment_state = {};
    colour_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

    VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {};
    colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colour_blend_state_create_info.attachmentCount = 1;
    colour_blend_state_create_info.pAttachments = &colour_blend_attachment_state;

    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (!_descriptor_set_layouts.empty())
    {
        layout_create_info.setLayoutCount = (uint32_t)_descriptor_set_layouts.size();
        layout_create_info.pSetLayouts = _descriptor_set_layouts.data();
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = (uint32_t)shader_stages.size();
    pipeline_create_info.pStages = shader_stages.data();
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &colour_blend_state_create_info;
    pipeline_create_info.subpass = _subpass;

    return pipeline.initialise(*_device, *_swapchain, *_render_pass, pipeline_create_info, layout_create_info);
}
