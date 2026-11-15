#include "vulkan_backend.h"

#include "vulkan_types.inl"

#include "core/logger.h"

static vulkan_context context;

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
    context.allocator = 0;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Dietlemon Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    const char* required_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(DPLATFORM_MAC) || defined(DPLATFORM_APPLE)
        "VK_EXT_metal_surface",
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
    };

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = sizeof(required_extensions) / sizeof(required_extensions[0]);
    create_info.ppEnabledExtensionNames = required_extensions;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = 0;

#if defined(DPLATFORM_MAC) || defined(DPLATFORM_APPLE)
    create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);
    if (result != VK_SUCCESS) {
        DERROR("vkCreateInstance() failed with result: %u", result);
        return false;
    }

    DINFO("Vulkan renderer initialized successfully");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {

}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height) {

}

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, float delta_time) {
    return true;
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, float delta_time) {
    return true;
}