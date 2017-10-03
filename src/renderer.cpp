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

    if (!_device.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, &_command_pool))
    {
        return false;
    }

    if (!_swapchain.initialise(_device))
    {
        return false;
    }

    if (!_render_pass.initialise(_device, _swapchain))
    {
        return false;
    }

    _graphics_pipeline_factory.initialise(_device, _swapchain, _render_pass, 0);

    _vertex_shader = _shader_cache.load("..\\res\\shaders\\triangle.vert.spv");
    _fragment_shader = _shader_cache.load("..\\res\\shaders\\triangle.frag.spv");

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

    _valid_state = true;

    return true;
}

void Renderer::shutdown()
{
    invalidate();

    if (_ubo)
    {
        vkDestroyBuffer((VkDevice)_device, _ubo, nullptr);
    }

    if (_ubo_memory)
    {
        vkFreeMemory((VkDevice)_device, _ubo_memory, nullptr);
    }

    _graphics_pipeline.destroy();

    if (_descriptor_pool)
    {
        vkDestroyDescriptorPool((VkDevice)_device, _descriptor_pool, nullptr);
    }

    if (_descriptor_set_layout)
    {
        vkDestroyDescriptorSetLayout((VkDevice)_device, _descriptor_set_layout, nullptr);
    }

    _vertex_buffer.destroy();
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

    if (!_swapchain.create(&width, &height, true))
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

    return true;
}

bool Renderer::draw_frame()
{
    if (!_valid_state)
    {
        return false;
    }

    if (!_swapchain.begin_frame())
    {
        return false;
    }

    void* data;
    VK_CHECK_RESULT(vkMapMemory((VkDevice)_device, _ubo_memory, 0, sizeof(UBO), 0, &data));
    memcpy(data, &_ubo_data, sizeof(UBO));
    vkUnmapMemory((VkDevice)_device, _ubo_memory);

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

    VkClearValue clear_value = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = (VkRenderPass)_render_pass;
    render_pass_begin_info.framebuffer = _frame_buffers[_swapchain.get_acquired_image_index()];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain.get_extent();
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)_graphics_pipeline);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)_graphics_pipeline, 0, 1, &_descriptor_set, 0,
                            nullptr);

    VkBuffer vertex_buffer = (VkBuffer)_vertex_buffer;
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offsets);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

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
    create_info.attachmentCount = 1;
    create_info.width = _swapchain.get_extent().width;
    create_info.height = _swapchain.get_extent().height;
    create_info.layers = 1;

    for (VkImageView image_view : _swapchain.get_image_views())
    {
        create_info.pAttachments = &image_view;

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
    static Vertex vertex_data[3] = { { { 0.0f, -0.375f }, { 1.0f, 0.0f, 0.0f } },
                                     { { 0.5f, 0.375f }, { 0.0f, 1.0f, 0.0f } },
                                     { { -0.5f, 0.375f }, { 0.0f, 0.0f, 1.0f } } };

    static VertexDecl decl = { { 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position), sizeof(glm::vec2) },
                               { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, colour), sizeof(glm::vec3) } };

    _graphics_pipeline_factory.set_vertex_decl(decl);

    if (!_vertex_buffer.create(_device, decl, 3))
    {
        return false;
    }

    void* mapped_memory;

    if (!_vertex_buffer.map(&mapped_memory))
    {
        return false;
    }

    memcpy(mapped_memory, vertex_data, sizeof(vertex_data));
    _vertex_buffer.unmap();

    return true;
}

bool Renderer::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_binding = {};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_binding;

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout((VkDevice)_device, &layout_info, nullptr, &_descriptor_set_layout));

    _graphics_pipeline_factory.add_descriptor_set_layout(_descriptor_set_layout);

    return true;
}

bool Renderer::create_descriptor_set()
{
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = 1;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;
    VK_CHECK_RESULT(vkCreateDescriptorPool((VkDevice)_device, &create_info, nullptr, &_descriptor_pool));

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = _descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &_descriptor_set_layout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets((VkDevice)_device, &alloc_info, &_descriptor_set));

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = _ubo;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UBO);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptor_set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets((VkDevice)_device, 1, &write, 0, nullptr);

    return true;
}

bool Renderer::create_ubo()
{
    return _device.create_buffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _ubo, _ubo_memory);
}