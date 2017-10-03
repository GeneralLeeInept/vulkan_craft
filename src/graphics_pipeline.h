#pragma once

#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "vertex_buffer.h"

class RenderPass;
class VulkanDevice;
class Swapchain;

class GraphicsPipeline
{
public:
    bool initialise(VulkanDevice& device, Swapchain& swapchain, RenderPass& render_pass, VkGraphicsPipelineCreateInfo& pipeline_create_info,
                    VkPipelineLayoutCreateInfo& layout_create_info);
    void destroy();

    bool create();
    void invalidate();

    explicit operator VkPipeline() { return _pipeline; }

private:
    std::vector<VkVertexInputBindingDescription> _vertex_bindings;
    std::vector<VkVertexInputAttributeDescription> _vertex_attributes;
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    std::vector<VkPipelineColorBlendAttachmentState> _attachments;
    VkPipelineVertexInputStateCreateInfo _vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo _input_assembly_state;
    VkPipelineRasterizationStateCreateInfo _rasterisation_state;
    VkPipelineMultisampleStateCreateInfo _multisample_state;
    VkPipelineDepthStencilStateCreateInfo _depth_stencil_state;
    VkPipelineColorBlendStateCreateInfo _colour_blend_state;
    VkGraphicsPipelineCreateInfo _pipeline_create_info = {};
    VkPipelineLayoutCreateInfo _layout_create_info = {};
    VulkanDevice* _device;
    RenderPass* _render_pass;
    Swapchain* _swapchain;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
};

class GraphicsPipelineFactory
{
public:
    void initialise(VulkanDevice& device, Swapchain& swapchain, RenderPass& render_pass, uint32_t subpass);

    void set_shader(VkShaderStageFlagBits stage, VkShaderModule shader, const char* name);
    void set_input_assembly_state(VkPrimitiveTopology topology, bool primtive_restart);
    void set_vertex_decl(const VertexDecl& vertex_decl);
    bool create_pipeline(GraphicsPipeline& pipeline);

private:
    typedef std::pair<VkShaderModule, const char*> ShaderRef;
    typedef std::map<VkShaderStageFlagBits, ShaderRef> ShaderRefMap;
    ShaderRefMap _shader_stages;
    std::vector<VkVertexInputBindingDescription> _vertex_bindings;
    std::vector<VkVertexInputAttributeDescription> _vertex_attributes;
    VulkanDevice* _device = nullptr;
    Swapchain* _swapchain = nullptr;
    RenderPass* _render_pass = nullptr;
    uint32_t _subpass = 0;
    VkPrimitiveTopology _primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool _primitive_restart = false;
};
