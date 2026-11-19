
#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"

#include "core/dstring.h"
#include "containers/darray.h"
#include "platform/platform.h"
#include "core/logger.h"

#include <string.h>

static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            DERROR("[Vulkan] %s", callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            DWARN("[Vulkan] %s", callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            DINFO("[Vulkan] %s", callback_data->pMessage);
            break;

        default:
            DDEBUG("[Vulkan] %s", callback_data->pMessage);
            break;
    }

    return VK_FALSE;
}

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
    context.allocator = 0;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Dietlemon Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    const char** required_extensions = darray_create(const char*);

    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&required_extensions);

#if defined(DPLATFORM_MAC) || defined(DPLATFORM_APPLE)
    darray_push(required_extensions, &VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#if defined(_DEBUG)
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    DDEBUG("Required extensions:");

    uint32_t extension_count = darray_length(required_extensions);

    for (uint32_t i = 0; i < extension_count; ++i) {
        DDEBUG("  %s", required_extensions[i]);
    }
#endif

    VkInstanceCreateInfo create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    };

    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    const char** required_validation_layer_names = 0;
    uint32_t required_validation_layer_count = 0;

#if defined(_DEBUG)
    DINFO("Validation layers enabled. Enumerating...");

    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");

    required_validation_layer_count = darray_length(required_validation_layer_names);

    uint32_t available_layer_count = 0;

    VK_CHECK(vkEnumerateInstanceLayerProperties(
        &available_layer_count,
        0
    ));

    VkLayerProperties* available_layers =
        darray_reserve(VkLayerProperties, available_layer_count);

    VK_CHECK(vkEnumerateInstanceLayerProperties(
        &available_layer_count,
        available_layers
    ));

    for (uint32_t i = 0; i < required_validation_layer_count; ++i) {
        DINFO("Searching for layer: %s...",
            required_validation_layer_names[i]);

        bool found = false;

        for (uint32_t j = 0; j < available_layer_count; ++j) {
            if (strcmp(
                required_validation_layer_names[i],
                available_layers[j].layerName
            ) == 0) {
                found = true;
                DINFO("Found");
                break;
            }
        }

        if (!found) {
            DFATAL(
                "Required validation layer is missing: %s",
                required_validation_layer_names[i]
            );

            darray_destroy(available_layers);
            darray_destroy(required_validation_layer_names);
            darray_destroy(required_extensions);

            return false;
        }
    }

    DINFO("All required validation layers are present");

    darray_destroy(available_layers);
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

#if defined(DPLATFORM_MAC) || defined(DPLATFORM_APPLE)
    create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VK_CHECK(vkCreateInstance(
        &create_info,
        context.allocator,
        &context.instance
    ));

    DINFO("Vulkan instance created");

#if defined(_DEBUG)
    DDEBUG("Creating Vulkan debugger");

    VkDebugUtilsMessageSeverityFlagsEXT log_severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
    };

    debug_create_info.messageSeverity = log_severity;

    debug_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    debug_create_info.pfnUserCallback = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(
            context.instance,
            "vkCreateDebugUtilsMessengerEXT"
        );

    DASSERT_MSG(func, "Failed to load vkCreateDebugUtilsMessengerEXT");

    VK_CHECK(func(
        context.instance,
        &debug_create_info,
        context.allocator,
        &context.debug_messenger
    ));

    DDEBUG("Vulkan debugger created");
#endif

    darray_destroy(required_extensions);

    if (required_validation_layer_names) {
        darray_destroy(required_validation_layer_names);
    }

    DINFO("Vulkan renderer initialized successfully");

    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
#if defined(_DEBUG)
    if (context.debug_messenger && context.instance) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(
                context.instance,
                "vkDestroyDebugUtilsMessengerEXT"
            );

        if (destroy_func) {
            destroy_func(
                context.instance,
                context.debug_messenger,
                context.allocator
            );
        }

        context.debug_messenger = 0;
    }
#endif

    if (context.instance) {
        vkDestroyInstance(context.instance, context.allocator);
        context.instance = 0;
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
