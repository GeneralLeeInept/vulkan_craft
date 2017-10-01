#include "renderer.h"

#include <Windows.h>

#include <GLFW/glfw3.h>
#include <sstream>
#include <vector>

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

    _valid_state = true;

    return true;
}

void Renderer::shutdown()
{
    invalidate();

    _swapchain.destroy();

    if (_command_pool)
    {
        vkDestroyCommandPool((VkDevice)_device, _command_pool, nullptr);
    }

    if (_drawing_complete_semaphore)
    {
        vkDestroySemaphore((VkDevice)_device, _drawing_complete_semaphore, nullptr);
    }

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

    if (!create_render_pass())
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
    VULKAN_CHECK_RESULT(vkAllocateCommandBuffers((VkDevice)_device, &alloc_info, &command_buffer));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VULKAN_CHECK_RESULT(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkClearValue clear_value = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.framebuffer = _frame_buffers[_swapchain.get_acquired_image_index()];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain.get_extent();
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer);

    VULKAN_CHECK_RESULT(vkEndCommandBuffer(command_buffer));

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

        if (_render_pass)
        {
            vkDestroyRenderPass((VkDevice)_device, _render_pass, nullptr);
            _render_pass = nullptr;
        }
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

    VULKAN_CHECK_RESULT(vkCreateInstance(&create_info, nullptr, &_vulkan_instance));

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
        VULKAN_CHECK_RESULT(func(_vulkan_instance, &debug_report_create_info, nullptr, &_debug_report));
    }
#endif

    VULKAN_CHECK_RESULT(glfwCreateWindowSurface(_vulkan_instance, _window, nullptr, &_surface));

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
    VULKAN_CHECK_RESULT(vkEnumeratePhysicalDevices(_vulkan_instance, &count, nullptr));

    if (!count)
    {
        return false;
    }

    std::vector<VkPhysicalDevice> devices(count);
    VULKAN_CHECK_RESULT(vkEnumeratePhysicalDevices(_vulkan_instance, &count, devices.data()));

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
    VULKAN_CHECK_RESULT(vkCreateSemaphore((VkDevice)_device, &create_info, nullptr, &_drawing_complete_semaphore));
    return true;
}

bool Renderer::create_command_pool()
{
    VULKAN_CHECK_RESULT(_device.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, &_command_pool));
    return true;
}

bool Renderer::create_render_pass()
{
    // Single pass forward-renderer
    VkAttachmentDescription colour_buffer = {};
    colour_buffer.format = _swapchain.get_image_format();
    colour_buffer.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_buffer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_buffer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_buffer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_buffer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_buffer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_buffer.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_buffer_reference = {};
    colour_buffer_reference.attachment = 0;
    colour_buffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_buffer_reference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &colour_buffer;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 0;
    create_info.pDependencies = nullptr;

    VULKAN_CHECK_RESULT(vkCreateRenderPass((VkDevice)_device, &create_info, nullptr, &_render_pass));

    return true;
}

bool Renderer::create_frame_buffers()
{
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = _render_pass;
    create_info.attachmentCount = 1;
    create_info.width = _swapchain.get_extent().width;
    create_info.height = _swapchain.get_extent().height;
    create_info.layers = 1;

    for (VkImageView image_view : _swapchain.get_image_views())
    {
        create_info.pAttachments = &image_view;

        VkFramebuffer frame_buffer;
        VULKAN_CHECK_RESULT(vkCreateFramebuffer((VkDevice)_device, &create_info, nullptr, &frame_buffer));
        _frame_buffers.push_back(frame_buffer);
    }

    return true;
}
