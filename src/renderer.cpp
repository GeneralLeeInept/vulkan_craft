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

    if (!_device.create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, _command_pool))
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

    _vertex_shader = _shader_cache.load(L"res/shaders/triangle.vert.spv");
    _fragment_shader = _shader_cache.load(L"res/shaders/triangle.frag.spv");

    if (!_vertex_shader && !_fragment_shader)
    {
        return false;
    }

    _graphics_pipeline_factory.set_shader(VK_SHADER_STAGE_VERTEX_BIT, _vertex_shader, "main");
    _graphics_pipeline_factory.set_shader(VK_SHADER_STAGE_FRAGMENT_BIT, _fragment_shader, "main");

    if (!_textures.create(_device, L"res/textures"))
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

    if (!_device.upload_texture(_textures))
    {
        return false;
    }

    _valid_state = true;

    return true;
}

void Renderer::shutdown()
{
    invalidate();

    _textures.destroy();
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

    _meshes.clear();

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

        uint32_t swapchain_image_count = (uint32_t)_swapchain.get_image_views().size();

        if (!create_command_buffers(swapchain_image_count))
        {
            return false;
        }

        if (!create_fences(swapchain_image_count))
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

Renderer::RenderMesh::RenderMesh(VkDevice device)
    : device(device)
{
}

Renderer::RenderMesh::RenderMesh(RenderMesh&& other)
{
    device = other.device;
    index_buffer = other.index_buffer;
    vertex_buffer = other.vertex_buffer;
    index_count = other.index_count;
    memory = other.memory;
    _aabb = other._aabb;
    memset(&other, 0, sizeof(RenderMesh));
}

Renderer::RenderMesh::~RenderMesh()
{
    if (index_buffer)
    {
        vkDestroyBuffer(device, index_buffer, nullptr);
    }

    if (vertex_buffer)
    {
        vkDestroyBuffer(device, vertex_buffer, nullptr);
    }

    if (memory)
    {
        vkFreeMemory(device, memory, nullptr);
    }
}

bool Renderer::add_mesh(const Mesh& mesh)
{
    uint32_t vertex_count = (uint32_t)mesh.vertices.size();

    if (vertex_count == 0)
    {
        return true;
    }

    RenderMesh render_mesh((VkDevice)_device);
    render_mesh.index_count = (uint32_t)mesh.indices.size();

    if (render_mesh.index_count == 0)
    {
        return true;
    }

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = vertex_count * sizeof(mesh.vertices[0]);
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer((VkDevice)_device, &create_info, nullptr, &render_mesh.vertex_buffer));

    create_info.size = render_mesh.index_count * sizeof(mesh.indices[0]);
    create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer((VkDevice)_device, &create_info, nullptr, &render_mesh.index_buffer));

    VkMemoryRequirements vertex_memory_requirements;
    vkGetBufferMemoryRequirements((VkDevice)_device, render_mesh.vertex_buffer, &vertex_memory_requirements);

    VkMemoryRequirements index_memory_requirements;
    vkGetBufferMemoryRequirements((VkDevice)_device, render_mesh.index_buffer, &index_memory_requirements);

    VkMemoryRequirements memory_requirements;
    memory_requirements.alignment = max(vertex_memory_requirements.alignment, index_memory_requirements.alignment);
    VkDeviceSize index_data_offset =
            (vertex_memory_requirements.size + index_memory_requirements.alignment - 1) &
            ~(index_memory_requirements.alignment - 1); // TODO: Fail on non-power of 2 alignments (like that would ever happen?)
    memory_requirements.size = index_data_offset + index_memory_requirements.size;
    memory_requirements.memoryTypeBits = vertex_memory_requirements.memoryTypeBits & index_memory_requirements.memoryTypeBits;

    if (memory_requirements.memoryTypeBits == 0)
    {
        return false;
    }

    VulkanBuffer staging_buffer;
    if (!staging_buffer.create(_device, memory_requirements.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        return false;
    }

    uint8_t* memory;

    if (!staging_buffer.map((void**)&memory))
    {
        return false;
    }

    memcpy(memory, mesh.vertices.data(), sizeof(mesh.vertices[0]) * mesh.vertices.size());
    memcpy(memory + index_data_offset, mesh.indices.data(), sizeof(mesh.indices[0]) * mesh.indices.size());
    staging_buffer.unmap();

    VkDeviceSize offset;
    if (!_device.allocate_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements, render_mesh.memory, offset))
    {
        return false;
    }

    VK_CHECK_RESULT(vkBindBufferMemory((VkDevice)_device, render_mesh.vertex_buffer, render_mesh.memory, offset));
    VK_CHECK_RESULT(vkBindBufferMemory((VkDevice)_device, render_mesh.index_buffer, render_mesh.memory, offset + index_data_offset));

    if (!_device.copy_buffer(staging_buffer, render_mesh.vertex_buffer, 0, 0, vertex_memory_requirements.size) ||
        !_device.copy_buffer(staging_buffer, render_mesh.index_buffer, index_data_offset, 0, index_memory_requirements.size))
    {
        return false;
    }

    render_mesh._aabb = mesh.aabb;

    staging_buffer.destroy();

    _meshes.push_back(std::move(render_mesh));

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

    uint32_t swapchain_image_index = _swapchain.get_acquired_image_index();

    void* data;
    if (!_ubo_buffer.map(&data))
    {
        return false;
    }
    memcpy(data, &_ubo_data, sizeof(UBO));
    _ubo_buffer.unmap();

    VkFence frame_fence = _frame_fences[swapchain_image_index];
    VK_CHECK_RESULT(vkWaitForFences((VkDevice)_device, 1, &frame_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences((VkDevice)_device, 1, &frame_fence));

    VkCommandBuffer command_buffer = _command_buffers[swapchain_image_index];
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
    render_pass_begin_info.framebuffer = _frame_buffers[swapchain_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain.get_extent();
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)_graphics_pipeline);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)_graphics_pipeline, 0, 1, &_descriptor_set, 0,
                            nullptr);

    geometry::frustum f;
    f.set_from_matrix(_ubo_data.proj * _ubo_data.view);
    for (const RenderMesh& mesh : _meshes)
    {
        if (culling::cull(f, mesh._aabb))
        {
            //continue;
        }

        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &mesh.vertex_buffer, &offsets);
        vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, mesh.index_count, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(command_buffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffer));

    VkSemaphore image_acquired_semaphore = _swapchain.get_image_acquired_semaphore();
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    _device.submit(command_buffer, 1, &image_acquired_semaphore, &wait_stage_mask, 1, &_drawing_complete_semaphore, frame_fence);

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

        for (VkFence& fence : _frame_fences)
        {
            vkDestroyFence((VkDevice)_device, fence, nullptr);
        }

        _frame_fences.clear();

        for (VkFramebuffer& framebuffer : _frame_buffers)
        {
            vkDestroyFramebuffer((VkDevice)_device, framebuffer, nullptr);
        }

        _frame_buffers.clear();

        if (_command_buffers.size())
        {
            vkFreeCommandBuffers((VkDevice)_device, _command_pool, (uint32_t)_command_buffers.size(), _command_buffers.data());
            _command_buffers.clear();
        }

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
    create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&create_info.enabledExtensionCount);
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

bool Renderer::create_command_buffers(uint32_t count)
{
    _command_buffers.resize(count);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = _command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;
    VK_CHECK_RESULT(vkAllocateCommandBuffers((VkDevice)_device, &alloc_info, _command_buffers.data()));
    return true;
}

bool Renderer::create_fences(uint32_t count)
{
    _frame_fences.resize(count);

    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (VkFence& fence : _frame_fences)
    {
        VK_CHECK_RESULT(vkCreateFence((VkDevice)_device, &create_info, nullptr, &fence));
    }

    return true;
}

bool Renderer::create_graphics_pipeline()
{
    static VertexDecl decl = { { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position), sizeof(glm::vec3) },
                               { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3) },
                               { 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex_coord), sizeof(glm::vec3) } };

    _graphics_pipeline_factory.set_vertex_decl(decl);

    return _graphics_pipeline_factory.create_pipeline(_graphics_pipeline);
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
    image_info.sampler = _textures._sampler;
    image_info.imageView = _textures._image_view;
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
