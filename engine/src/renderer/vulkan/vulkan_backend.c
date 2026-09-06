#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_command_buffer.h"

#include "core/logger.h"
#include "containers/darray.h"
#include "platform/platform.h"
#include "core/application.h"

#include <string.h>

static vulkan_context context;
static uint32_t cached_framebuffer_width = 0;
static uint32_t cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags);
void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
    context.find_memory_index = find_memory_index;
    
    context.allocator = 0;

    application_get_framebuffer_size(&cached_framebuffer_width, &cached_framebuffer_height);
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;
    
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Dietlemon Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&required_extensions);
#if defined(_DEBUG)
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    DDEBUG("Required extensions:");
    uint32_t length = darray_length(required_extensions);
    for (uint32_t i = 0; i < length; ++i) {
        DDEBUG(required_extensions[i]);
    }
#endif

    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    const char** required_validation_layer_names = 0;
    uint32_t required_validation_layer_count = 0;

#if defined(_DEBUG)
    DINFO("Validation layer enabled time to enumerate");

    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    uint32_t available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (uint32_t i = 0; i < required_validation_layer_count; ++i) {
        DINFO("Searching for layer: %s...", required_validation_layer_names[i]);
        bool found = false;
        for (uint32_t j = 0; j < available_layer_count; ++j) {
            if (strcmp(required_validation_layer_names[i], available_layers[j].layerName) == 0) {
                found = true;
                DINFO("Found.");
                break;
            }
        }

        if (!found) {
            DFATAL("Required validation layer is missig: %s", required_validation_layer_names[i]);
            return false;
        }
    }
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    DINFO("Vulkan Instance created");

#if defined(_DEBUG)
    DDEBUG("Creating vulkan debugger...");
    uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
    
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    DASSERT_MSG(func, "Failed to create debug messenger");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    DDEBUG("Vulkan debugger created");
#endif

    DDEBUG("Creating Vulakn surface...");
    if (!platform_create_vulkan_surface(plat_state, &context)) {
        DERROR("Failed to create platform surface");;
        return false;
    }
    DDEBUG("Vulkan surface create");

    if (!vulkan_device_create(&context)) {
        DERROR("Failed to create device");
        return false;
    }

    vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain);
    vulkan_renderpass_create(&context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height, 0.0f, 0.0f, 0.2f, 0.1f, 1.0f, 0);

    context.swapchain.framebuffers = darray_reserve(vulkan_framebuffer, context.swapchain.image_count);
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);
    
    create_command_buffers(backend);

    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.in_flight_fences = darray_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);
    for (uint8_t i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);
        
        vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
    }
    
    context.images_in_flight = darray_reserve(vulkan_fence, context.swapchain.image_count);
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }
    
    DINFO("Vulkan renderer initialized successfully");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
    vkDeviceWaitIdle(context.device.logical_device);

    for (uint8_t i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        if (context.image_available_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical_device,
                context.image_available_semaphores[i],
                context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if (context.queue_complete_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical_device,
                context.queue_complete_semaphores[i],
                context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }
    darray_destroy(context.image_available_semaphores);
    context.image_available_semaphores = 0;

    darray_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;

    darray_destroy(context.in_flight_fences);
    context.in_flight_fences = 0;

    darray_destroy(context.images_in_flight);
    context.images_in_flight = 0;
    
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free(&context,
                                       context.device.graphics_command_pool,
                                       &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    darray_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }
    
    vulkan_renderpass_destroy(&context, &context.main_renderpass);
    vulkan_swapchain_destroy(&context, &context.swapchain);
    DDEBUG("Destroying vulkan device...");
    vulkan_device_destroy(&context);
    DDEBUG("Destroying vulkan surface...");
    if (context.surface) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }
    
    DDEBUG("Destroying vulkan debugger...");
    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height) {
}

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, float delta_time) {
    return true;
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, float delta_time) {
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            DERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            DWARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            DINFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            DTRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }

    DWARN("Unable to find suitable memory type");
    return -1;
}

void create_command_buffers(renderer_backend* backend) {
    if (!context.graphics_command_buffers) {
        context.graphics_command_buffers = darray_reserve(vulkan_command_buffer, context.swapchain.image_count);
        for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
            memset(&context.graphics_command_buffers[i], 0, sizeof(vulkan_command_buffer));
        }
    }

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free(
                &context,
                context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]);
        }
        memset(&context.graphics_command_buffers[i], 0, sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate(
            &context,
            context.device.graphics_command_pool,
            true,
            &context.graphics_command_buffers[i]);
    }

    DDEBUG("Vulkan command buffers created.");
}

void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass) {
    for (uint32_t i = 0; i < swapchain->image_count; ++i) {
        uint32_t attachment_count = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view};
        
        vulkan_framebuffer_create(
            &context,
            renderpass,
            context.framebuffer_width,
            context.framebuffer_height,
            attachment_count,
            attachments,
            &context.swapchain.framebuffers[i]);
    }
}
