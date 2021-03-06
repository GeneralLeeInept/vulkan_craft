#pragma once

#include <glm/mat4x4.hpp>

#include "culling.h"
#include "depth_buffer.h"
#include "graphics_pipeline.h"
#include "mesh_cache.h"
#include "render_pass.h"
#include "shader_cache.h"
#include "texture_cache.h"
#include "vertex_buffer.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

struct UBO
{
    glm::mat4x4 model;
    glm::mat4x4 view;
    glm::mat4x4 proj;
};

class Renderer
{
public:
    bool initialise(struct GLFWwindow* window);
    void shutdown();

    bool set_window_size(uint32_t width, uint32_t height);

    void set_model_matrix(glm::mat4x4& m) { _ubo_data.model = m; }
    void set_view_matrix(glm::mat4x4& m) { _ubo_data.view = m; }
    void set_proj_matrix(glm::mat4x4& m) { _ubo_data.proj = m; }
    bool add_mesh(const struct Mesh& mesh);

    bool draw_frame();

private:
    void invalidate();

    bool create_instance();
    bool create_device();
    bool create_semaphores();
    bool create_frame_buffers();
    bool create_command_buffers(uint32_t count);
    bool create_fences(uint32_t count);
    bool create_graphics_pipeline();
    bool create_descriptor_set_layout();
    bool create_descriptor_set();
    bool create_ubo();

    ShaderCache _shader_cache;
    VulkanDevice _device;
    Swapchain _swapchain;
    DepthBuffer _depth_buffer;
    RenderPass _render_pass;
    GraphicsPipelineFactory _graphics_pipeline_factory;
    GraphicsPipeline _graphics_pipeline;
    VulkanBuffer _ubo_buffer;
    std::vector<VkFramebuffer> _frame_buffers;
    std::vector<VkCommandBuffer> _command_buffers;
    std::vector<VkFence> _frame_fences;
    GLFWwindow* _window = nullptr;
    VkInstance _vulkan_instance = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkCommandPool _command_pool = VK_NULL_HANDLE;
    VkSemaphore _drawing_complete_semaphore = VK_NULL_HANDLE;
    VkShaderModule _vertex_shader = VK_NULL_HANDLE;
    VkShaderModule _fragment_shader = VK_NULL_HANDLE;

    UBO _ubo_data;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

    TextureArray _textures;

    std::vector<RenderMesh> _meshes;

    bool _valid_state = false;
};