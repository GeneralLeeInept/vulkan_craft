#include "renderer.h"

#include <Windows.h>

#include <GLFW/glfw3.h>
#include <sstream>
#include <vector>

#include "geometry.h"
#include "vulkan.h"

bool Renderer::initialise(GLFWwindow* window)
{
    _window = window;

    if (!create_instance())
    {
        return false;
    }

    if (!create_device())
    {
        return false;
    }

    if (!_shader_cache.initialise((VkDevice)_device))
    {
        return false;
    }

    if (!create_semaphores())
    {
        return false;
    }

    if (!_device.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, _command_pool))
    {
        return false;
    }

    if (!_swapchain.initialise(_device))
    {
        return false;
    }

    if (!_depth_buffer.initialise(_device, _swapchain))
    {
        return false;
    }

    if (!_render_pass.initialise(_device, _swapchain, &_depth_buffer))
    {
        return false;
    }

    _graphics_pipeline_factory.initialise(_device, _swapchain, _render_pass, 0);

    _vertex_shader = _shader_cache.load("res/shaders/triangle.vert.spv");
    _fragment_shader = _shader_cache.load("res/shaders/triangle.frag.spv");

    if (!_vertex_shader && !_fragment_shader)
    {
        return false;
    }

    _graphics_pipeline_factory.set_shader(VK_SHADER_STAGE_VERTEX_BIT, _vertex_shader, "main");
    _graphics_pipeline_factory.set_shader(VK_SHADER_STAGE_FRAGMENT_BIT, _fragment_shader, "main");

    if (!create_vertex_buffer())
    {
        return false;
    }

    if (!_texture.create(_device, "res/textures/orientation.png"))
    {
        return false;
    }

    if (!create_descriptor_set_layout())
    {
        return false;
    }

    if (!create_graphics_pipeline())
    {
        return false;
    }

    if (!create_ubo())
    {
        return false;
    }

    if (!create_descriptor_set())
    {
        return false;
    }

    if (!_device.upload_texture(_texture))
    {
        return false;
    }

    _valid_state = true;

    return true;
}

void Renderer::shutdown()
{
    invalidate();

    _texture.destroy();
    _ubo_buffer.destroy();
    _graphics_pipeline.destroy();

    if (_descriptor_pool)
    {
        vkDestroyDescriptorPool((VkDevice)_device, _descriptor_pool, nullptr);
    }

    if (_descriptor_set_layout)
    {
        vkDestroyDescriptorSetLayout((VkDevice)_device, _descriptor_set_layout, nullptr);
    }

    _index_buffer.destroy();
    _vertex_buffer.destroy();
    _depth_buffer.destroy();
    _swapchain.destroy();

    if (_command_pool)
    {
        vkDestroyCommandPool((VkDevice)_device, _command_pool, nullptr);
    }

    if (_drawing_complete_semaphore)
    {
        vkDestroySemaphore((VkDevice)_device, _drawing_complete_semaphore, nullptr);
    }

    _shader_cache.release(_vertex_shader);
    _shader_cache.release(_fragment_shader);

    _device.destroy();

    if (_surface)
    {
        vkDestroySurfaceKHR(_vulkan_instance, _surface, nullptr);
    }

    if (_debug_report)
    {
        PFN_vkDestroyDebugReportCallbackEXT func =
                (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_vulkan_instance, "vkDestroyDebugReportCallbackEXT");
        func(_vulkan_instance, _debug_report, nullptr);
    }

    if (_vulkan_instance)
    {
        vkDestroyInstance(_vulkan_instance, nullptr);
    }
}

bool Renderer::set_window_size(uint32_t width, uint32_t height)
{
    invalidate();

    if (width && height)
    {
        if (!_swapchain.create(&width, &height, true))
        {
            return false;
        }

        if (!_depth_buffer.create())
        {
            return false;
        }

        if (!_render_pass.create())
        {
            return false;
        }

        if (!_graphics_pipeline.create())
        {
            return false;
        }

        if (!create_frame_buffers())
        {
            return false;
        }

        _valid_state = true;
    }

    return true;
}

bool Renderer::draw_frame()
{
    if (!_valid_state)
    {
        return true;
    }

    if (!_swapchain.begin_frame())
    {
        return false;
    }

    void* data;
    if (!_ubo_buffer.map(&data))
    {
        return false;
    }
    memcpy(data, &_ubo_data, sizeof(UBO));
    _ubo_buffer.unmap();

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = _command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK_RESULT(vkAllocateCommandBuffers((VkDevice)_device, &alloc_info, &command_buffer));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK_RESULT(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkClearValue clear_values[2];
    clear_values[0] = { 0.6f, 0.0f, 0.6f, 1.0f };
    clear_values[1] = { 1.0f, 0 };
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = (VkRenderPass)_render_pass;
    render_pass_begin_info.framebuffer = _frame_buffers[_swapchain.get_acquired_image_index()];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain.get_extent();
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)_graphics_pipeline);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)_graphics_pipeline, 0, 1, &_descriptor_set, 0,
                            nullptr);

    VkBuffer vertex_buffer = (VkBuffer)_vertex_buffer;
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offsets);

    vkCmdBindIndexBuffer(command_buffer, (VkBuffer)_index_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, 36 * (16 * 16 + 1), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffer));

    VkSemaphore image_acquired_semaphore = _swapchain.get_image_acquired_semaphore();
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    _device.submit(command_buffer, 1, &image_acquired_semaphore, &wait_stage_mask, 1, &_drawing_complete_semaphore);

    if (!_swapchain.end_frame(1, &_drawing_complete_semaphore))
    {
        return false;
    }

    return true;
}

#if !defined(NDEBUG)
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location,
                                                     int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
    std::stringstream ss;
    ss << "validation layer: " << msg << std::endl;
    OutputDebugStringA(ss.str().c_str());
    return VK_FALSE;
}
#endif

void Renderer::invalidate()
{
    if (_valid_state)
    {
        _valid_state = false;

        vkDeviceWaitIdle((VkDevice)_device);

        for (VkFramebuffer& framebuffer : _frame_buffers)
        {
            vkDestroyFramebuffer((VkDevice)_device, framebuffer, nullptr);
        }

        _frame_buffers.clear();
        _graphics_pipeline.invalidate();
        _render_pass.invalidate();
        _depth_buffer.invalidate();
    }
}

bool Renderer::create_instance()
{
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "vulkan_craft";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;

#if defined(NDEBUG)
    instance_create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instance_create_info.enabledExtensionCount);
#else
    const char* validation_layer = "VK_LAYER_LUNARG_standard_validation";
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = &validation_layer;

    uint32_t glfw_extension_count;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extension_names;

    extension_names.reserve(glfw_extension_count + 1);

    for (uint32_t i = 0; i < glfw_extension_count; ++i)
    {
        extension_names.push_back(glfw_extensions[i]);
    }

    extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    create_info.enabledExtensionCount = glfw_extension_count + 1;
    create_info.ppEnabledExtensionNames = extension_names.data();
#endif

    VK_CHECK_RESULT(vkCreateInstance(&create_info, nullptr, &_vulkan_instance));

#if !defined(NDEBUG)
    PFN_vkCreateDebugReportCallbackEXT func =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_vulkan_instance, "vkCreateDebugReportCallbackEXT");

    if (func)
    {
        VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {};
        debug_report_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_create_info.flags =
                VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debug_report_create_info.pfnCallback = debug_callback;
        debug_report_create_info.pUserData = nullptr;
        VK_CHECK_RESULT(func(_vulkan_instance, &debug_report_create_info, nullptr, &_debug_report));
    }
#endif

    VK_CHECK_RESULT(glfwCreateWindowSurface(_vulkan_instance, _window, nullptr, &_surface));

    return true;
}

static bool check_device(VulkanDevice& device)
{
    if (device.find_queue_family_index(VK_QUEUE_GRAPHICS_BIT) == UINT32_MAX)
    {
        return false;
    }

    return true;
}

static VulkanDevice& compare_devices(VulkanDevice& a, VulkanDevice& b)
{
    return b;
}

static VulkanDevice pick_device(const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface)
{
    VulkanDevice best;

    for (VkPhysicalDevice d : devices)
    {
        VulkanDevice device;

        if (device.initialise(d, surface))
        {
            if (check_device(device))
            {
                best = std::move(compare_devices(best, device));
            }
        }
    }

    return best;
}

bool Renderer::create_device()
{
    uint32_t count;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_vulkan_instance, &count, nullptr));

    if (!count)
    {
        return false;
    }

    std::vector<VkPhysicalDevice> devices(count);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_vulkan_instance, &count, devices.data()));

    _device = pick_device(devices, _surface);

    if (!(VkPhysicalDevice)_device)
    {
        return false;
    }

    return _device.create();
}

bool Renderer::create_semaphores()
{
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore((VkDevice)_device, &create_info, nullptr, &_drawing_complete_semaphore));
    return true;
}

bool Renderer::create_frame_buffers()
{
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = (VkRenderPass)_render_pass;
    create_info.attachmentCount = 2;
    create_info.width = _swapchain.get_extent().width;
    create_info.height = _swapchain.get_extent().height;
    create_info.layers = 1;

    for (VkImageView image_view : _swapchain.get_image_views())
    {
        VkImageView attachments[2] = { image_view, _depth_buffer.get_image_view() };
        create_info.pAttachments = attachments;

        VkFramebuffer frame_buffer;
        VK_CHECK_RESULT(vkCreateFramebuffer((VkDevice)_device, &create_info, nullptr, &frame_buffer));
        _frame_buffers.push_back(frame_buffer);
    }

    return true;
}

bool Renderer::create_graphics_pipeline()
{
    return _graphics_pipeline_factory.create_pipeline(_graphics_pipeline);
}

bool Renderer::create_vertex_buffer()
{
    static VertexDecl decl = { { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position), sizeof(glm::vec3) },
                               { 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord), sizeof(glm::vec2) } };

    _graphics_pipeline_factory.set_vertex_decl(decl);

    Chunk chunk;
    chunk.generate();
    chunk.create_mesh();
    Mesh& mesh = chunk.mesh;

    if (!_vertex_buffer.create(_device, decl, (uint32_t)mesh.vertices.size()))
    {
        return false;
    }

    void* memory;

    if (!_vertex_buffer.map((void**)&memory))
    {
        return false;
    }

    memcpy(memory, mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size());
    _vertex_buffer.unmap();


    if (!_index_buffer.create(_device, sizeof(uint16_t) * mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        return false;
    }

    if (!_index_buffer.map(&memory))
    {
        return false;
    }

    memcpy(memory, mesh.indices.data(), sizeof(uint16_t) * mesh.indices.size());
    _index_buffer.unmap();

    return true;
}

bool Renderer::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding bindings[2];

    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout((VkDevice)_device, &layout_info, nullptr, &_descriptor_set_layout));

    _graphics_pipeline_factory.add_descriptor_set_layout(_descriptor_set_layout);

    return true;
}

bool Renderer::create_descriptor_set()
{
    VkDescriptorPoolSize pool_sizes[2];

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 1;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = 1;
    create_info.poolSizeCount = 2;
    create_info.pPoolSizes = pool_sizes;
    VK_CHECK_RESULT(vkCreateDescriptorPool((VkDevice)_device, &create_info, nullptr, &_descriptor_pool));

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = _descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &_descriptor_set_layout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets((VkDevice)_device, &alloc_info, &_descriptor_set));

    VkDescriptorBufferInfo buffer_info = _ubo_buffer.get_descriptor_info();

    VkDescriptorImageInfo image_info = {};
    image_info.sampler = _texture._sampler;
    image_info.imageView = _texture._image_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_writes[2];
    descriptor_writes[0] = {};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = _descriptor_set;
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pBufferInfo = &buffer_info;

    descriptor_writes[1] = {};
    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet = _descriptor_set;
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].pImageInfo = &image_info;

    vkUpdateDescriptorSets((VkDevice)_device, 2, descriptor_writes, 0, nullptr);

    return true;
}

bool Renderer::create_ubo()
{
    return _ubo_buffer.create(_device, sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}