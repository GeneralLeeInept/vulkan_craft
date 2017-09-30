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

    _swapchain.initialise(_device);

    return true;
}

void Renderer::shutdown()
{
    _swapchain.destroy();
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
    vkDeviceWaitIdle((VkDevice)_device);
    return _swapchain.create(&width, &height, true);
}

bool Renderer::draw_frame()
{
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
