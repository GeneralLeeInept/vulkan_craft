#pragma once

#include <vector>
#include <vulkan/vulkan.h>

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
