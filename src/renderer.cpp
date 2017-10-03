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

    if (!create_command_pool())
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

    _vertex_shader = _shader_cache.load("..\\res\\shaders\\triangle.vert.spv");
    _fragment_shader = _shader_cache.load("..\\res\\shaders\\triangle.frag.spv");

    if (!_vertex_shader && !_fragment_shader)
    {
        return false;
    }

    if (!create_graphics_pipeline())
    {
        return false;
    }

    if (!create_vertex_buffer())
    {
        return false;
    }

    _valid_state = true;

    return true;
}

void Renderer::shutdown()
{
    invalidate();

    if (_vertex_buffer)
    {
        vkDestroyBuffer((VkDevice)_device, _vertex_buffer, nullptr);
    }

    if (_vertex_buffer_memory)
    {
        vkFreeMemory((VkDevice)_device, _vertex_buffer_memory, nullptr);
    }

    _graphics_pipeline.destroy();

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
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer, &offsets);
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

bool Renderer::create_command_pool()
{
    VK_CHECK_RESULT(_device.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, &_command_pool));
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
    VkPipelineShaderStageCreateInfo shader_stages[2];
    shader_stages[0] = {};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = _vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[1] = {};
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = _fragment_shader;
    shader_stages[1].pName = "main";

    VkVertexInputBindingDescription vertex_binding = {};
    vertex_binding.binding = 0;
    vertex_binding.stride = sizeof(Vertex);
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[2];
    vertex_attributes[0].location = 0;
    vertex_attributes[0].binding = 0;
    vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attributes[0].offset = offsetof(Vertex, position);
    vertex_attributes[1].location = 1;
    vertex_attributes[1].binding = 0;
    vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[1].offset = offsetof(Vertex, colour);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

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

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = 2;
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input_state_create_info;
    create_info.pInputAssemblyState = &input_assembly_state_create_info;
    create_info.pRasterizationState = &rasterization_state_create_info;
    create_info.pMultisampleState = &multisample_state_create_info;
    create_info.pDepthStencilState = &depth_stencil_state_create_info;
    create_info.pColorBlendState = &colour_blend_state_create_info;

    return _graphics_pipeline.initialise(_device, _swapchain, _render_pass, create_info, pipeline_layout_create_info);
}

bool Renderer::create_vertex_buffer()
{
    static Vertex vertex_data[3] = { { { 0.0f, -0.375f }, { 1.0f, 0.0f, 0.0f } },
                                     { { 0.5f, 0.375f }, { 0.0f, 1.0f, 0.0f } },
                                     { { -0.5f, 0.375f }, { 0.0f, 0.0f, 1.0f } } };

    static uint32_t vertex_data_size = (uint32_t)sizeof(vertex_data);

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = vertex_data_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer((VkDevice)_device, &buffer_create_info, nullptr, &_vertex_buffer));

    const VkPhysicalDeviceMemoryProperties& memory_properties = _device.get_memory_properties();
    uint32_t memory_type_index;
    VkMemoryPropertyFlags desired_properties =
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; ++memory_type_index)
    {
        if ((memory_properties.memoryTypes[memory_type_index].propertyFlags & desired_properties) == desired_properties)
        {
            break;
        }
    }

    if (memory_type_index == memory_properties.memoryTypeCount)
    {
        return false;
    }

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = vertex_data_size;
    allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK_RESULT(vkAllocateMemory((VkDevice)_device, &allocate_info, nullptr, &_vertex_buffer_memory));

    VK_CHECK_RESULT(vkBindBufferMemory((VkDevice)_device, _vertex_buffer, _vertex_buffer_memory, 0));

    void* mapped_memory;
    VK_CHECK_RESULT(vkMapMemory((VkDevice)_device, _vertex_buffer_memory, 0, vertex_data_size, 0, &mapped_memory));
    memcpy(mapped_memory, vertex_data, vertex_data_size);
    vkUnmapMemory((VkDevice)_device, _vertex_buffer_memory);

    return true;
}
