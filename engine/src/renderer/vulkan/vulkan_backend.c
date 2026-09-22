#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"
#include "vulkan_buffer.h"
#include "vulkan_image.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/application.h"
#include "containers/darray.h"
#include "math/math_types.h"
#include "platform/platform.h"
#include "shaders/vulkan_object_shader.h"

#include <string.h>
#include <stdlib.h>

static vulkan_context context;
static uint32_t cached_framebuffer_width = 0;
static uint32_t cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags);
bool create_buffers(vulkan_context* context);
void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);
bool recreate_swapchain(renderer_backend* backend);

void upload_data_range(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, uint64_t offset, uint64_t size, void* data) {
  VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  vulkan_buffer staging;
  vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);
  vulkan_buffer_load_data(context, &staging, 0, size, 0, data);
  vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size);
  vulkan_buffer_destroy(context, &staging);
}

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name) {
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
    DINFO("Validation layers enabled. Enumerating...");
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
            DFATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return false;
        }
    }
    DINFO("All required validation layers are present.");
#endif
    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;
    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    DINFO("Vulkan Instance created.");
#if defined(_DEBUG)
    DDEBUG("Creating Vulkan debugger...");
    uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    DASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    DDEBUG("Vulkan debugger created.");
#endif
    DDEBUG("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(&context)) {
        DERROR("Failed to create platform surface!");
        return false;
    }
    DDEBUG("Vulkan surface created.");
    if (!vulkan_device_create(&context)) {
        DERROR("Failed to create device!");
        return false;
    }
    vulkan_swapchain_create(
        &context,
        context.framebuffer_width,
        context.framebuffer_height,
        &context.swapchain);
    vulkan_renderpass_create(
        &context,
        &context.main_renderpass,
        0, 0, context.framebuffer_width, context.framebuffer_height,
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f,
        0);
    context.swapchain.framebuffers = darray_reserve(vulkan_framebuffer, context.swapchain.image_count);
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);
    create_command_buffers(backend);
    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.image_count);
    context.in_flight_fences = darray_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);

    for (uint8_t i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(
            context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.image_available_semaphores[i]);
        vulkan_fence_create(
            &context,
            true,
            &context.in_flight_fences[i]);
    }
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(
            context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.queue_complete_semaphores[i]);
    }
    context.images_in_flight = darray_reserve(vulkan_fence, context.swapchain.image_count);
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }
    if (!vulkan_object_shader_create(&context, &context.object_shader)) {
      DERROR("Error loading built-in basic_lighting shader");
      return false;
    }
    
    create_buffers(&context);
    
    const uint32_t vert_count = 4;
    vertex_3d verts[4];
    memset(verts, 0, sizeof(verts));
    const float f = 10.0f;
    verts[0].position.x = -0.5 * f;
    verts[0].position.y = -0.5 * f;
    verts[1].position.y = 0.5 * f;
    verts[1].position.x = 0.5 * f;
    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    
    const uint32_t index_count = 6;
    uint32_t indices[6] = {0, 1, 2, 0, 3, 1};
    upload_data_range(&context, context.device.graphics_command_pool, 0,
                      context.device.graphics_queue,
                      &context.object_vertex_buffer, 0,
                      sizeof(vertex_3d) * vert_count, verts);
    upload_data_range(&context, context.device.graphics_command_pool, 0,
                      context.device.graphics_queue,
                      &context.object_index_buffer, 0,
                      sizeof(uint32_t) * index_count, indices);
    
    DINFO("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
    vkDeviceWaitIdle(context.device.logical_device);
    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);
    
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
            vulkan_command_buffer_free(
                &context,
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
    DDEBUG("Destroying Vulkan device...");
    vulkan_device_destroy(&context);
    DDEBUG("Destroying Vulkan surface...");
    if (context.surface) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }
#if (_DEBUG)
    DDEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }
#endif
    
    DDEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height) {
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;
    context.framebuffer_size_generation++;
    DINFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%llu", width, height, context.framebuffer_size_generation);
}

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, float delta_time) {
    vulkan_device* device = &context.device;
    if (context.recreating_swapchain) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result)) {
            DERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }
        DINFO("Recreating swapchain, booting.");
        return false;
    }
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result)) {
            DERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }
        if (!recreate_swapchain(backend)) {
            return false;
        }
        DINFO("Resized, booting.");
        return false;
    }
    if (!vulkan_fence_wait(
            &context,
            &context.in_flight_fences[context.current_frame],
            UINT64_MAX)) {
        DERROR("In-flight fence wait failure - DEVICE LOST. Shutting down.");
        exit(1);
    }
    if (!vulkan_swapchain_acquire_next_image_index(
            &context,
            &context.swapchain,
            UINT64_MAX,
            context.image_available_semaphores[context.current_frame],
            0,
            &context.image_index)) {
        return false;
    }
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (float)context.framebuffer_height;
    viewport.width = (float)context.framebuffer_width;
    viewport.height = -(float)context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    vulkan_renderpass_begin(command_buffer,
                            &context.main_renderpass,
                            context.swapchain.framebuffers[context.image_index].handle);
    return true;
}

void vulkan_renderer_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, int32_t mode) {
  vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
  vulkan_object_shader_use(&context, &context.object_shader);
  
  context.object_shader.global_ubo.projection = projection;
  context.object_shader.global_ubo.view = view;
  
  vulkan_object_shader_update_global_state(&context, &context.object_shader);
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, float delta_time) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_renderpass_end(command_buffer, &context.main_renderpass);
    vulkan_command_buffer_end(command_buffer);
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {
        vulkan_fence_wait(
            &context,
            context.images_in_flight[context.image_index],
            UINT64_MAX);
    }
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.image_index];
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;
    VkResult result = vkQueueSubmit(
        context.device.graphics_queue,
        1,
        &submit_info,
        context.in_flight_fences[context.current_frame].handle);
    if (result != VK_SUCCESS) {
        DERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, true));
        return false;
    }
    vulkan_command_buffer_update_submitted(command_buffer);
    vulkan_swapchain_present(
        &context,
        &context.swapchain,
        context.device.graphics_queue,
        context.device.present_queue,
        context.queue_complete_semaphores[context.image_index],
        context.image_index);
    return true;
}

void vulkan_backend_update_object(mat4 model) {
  vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
  vulkan_object_shader_update_object(&context, &context.object_shader, model);
  vulkan_object_shader_use(&context, &context.object_shader);
  VkDeviceSize offset[1] = {0};
  vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context.object_vertex_buffer.handle, (VkDeviceSize*)offset);
  vkCmdBindIndexBuffer(command_buffer->handle, context.object_index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(command_buffer->handle, 6, 1, 0, 0, 0);
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
    DWARN("Unable to find suitable memory type!");
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

bool recreate_swapchain(renderer_backend* backend) {
    if (context.recreating_swapchain) {
        DDEBUG("recreate_swapchain called when already recreating. Booting.");
        return false;
    }
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        DDEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return false;
    }
    context.recreating_swapchain = true;
    vkDeviceWaitIdle(context.device.logical_device);
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }
    vulkan_device_query_swapchain_support(
        context.device.physical_device,
        context.surface,
        &context.device.swapchain_support);
    vulkan_device_detect_depth_format(&context.device);
    vulkan_swapchain_recreate(
        &context,
        cached_framebuffer_width,
        cached_framebuffer_height,
        &context.swapchain);
    context.framebuffer_width = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }
    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);
    create_command_buffers(backend);

    vulkan_object_shader_destroy(&context, &context.object_shader);
    if (!vulkan_object_shader_create(&context, &context.object_shader)) {
        DERROR("Failed to recreate object shader after swapchain recreate");
        context.recreating_swapchain = false;
        return false;
    }

    context.recreating_swapchain = false;
    return true;
}

bool create_buffers(vulkan_context* context) {
  VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  
  const uint64_t vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
  if (!vulkan_buffer_create(context, 
                            vertex_buffer_size, 
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            memory_property_flags,
                            true,
                            &context->object_vertex_buffer)) {
    DERROR("Error creating vertex buffer");
    return false;
  }
  context->geometry_vertex_offset = 0;
  
  const uint64_t index_buffer_size = sizeof(uint32_t) * 1024 * 1024;
  if (!vulkan_buffer_create(context, 
                            index_buffer_size, 
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            memory_property_flags,
                            true,
                            &context->object_index_buffer)) {
    DERROR("Error creating vertex buffer");
    return false;
  }
  context->geometry_index_offset = 0;
  return true;
}

void vulkan_renderer_create_texture(const char* name, bool auto_release, int32_t width, int32_t height, int32_t channel_count, const uint8_t* pixels, bool has_transparency, texture* out_texture) {
  out_texture->width = width;
  out_texture->height = height;
  out_texture->channel_count = channel_count;
  out_texture->generation = 0;
  out_texture->internal_data = (vulkan_texture_data*)dallocate(sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
  vulkan_texture_data* data = (vulkan_texture_data*)out_texture->internal_data;
  VkDeviceSize image_size = width * height * channel_count;
  
  VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;
  
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  
  vulkan_buffer staging;
  vulkan_buffer_create(&context, image_size, usage, memory_prop_flags, true, &staging);
  vulkan_buffer_load_data(&context, &staging, 0, image_size, 0, pixels);
  vulkan_image_create(&context, VK_IMAGE_TYPE_2D, width, height, image_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      VK_IMAGE_ASPECT_COLOR_BIT,
                      &data->image);
  vulkan_command_buffer temp_buffer;
  VkCommandPool pool = context.device.graphics_command_pool;
  VkQueue queue = context.device.graphics_queue;
  vulkan_command_buffer_allocate_and_begin_single_use(&context, pool, &temp_buffer);
  
  vulkan_image_transition_layout(&context, &temp_buffer, &data->image, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan_image_copy_from_buffer(&context, &data->image, staging.handle, &temp_buffer);
  vulkan_image_transition_layout(&context, &temp_buffer, &data->image, image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vulkan_command_buffer_end_single_use(&context, pool, &temp_buffer, queue);
  
  VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_TRUE;
  sampler_info.maxAnisotropy = 16;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;
  
  VkResult result = vkCreateSampler(context.device.logical_device, &sampler_info, context.allocator, &data->sampler);
  if (!vulkan_result_is_success(VK_SUCCESS)) {
    DERROR("Error creating texture sampler: %s", vulkan_result_string(result, true));
    return;
  }
  
  out_texture->has_transparency = has_transparency;
  out_texture->generation++;
}

void vulkan_renderer_destroy_texture(struct texture* texture) {
  vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;
  
  vulkan_image_destroy(&context, &data->image);
  memset(&data->image, 0, sizeof(vulkan_image));
  vkDestroySampler(context.device.logical_device, data->sampler, context.allocator);
  data->sampler = 0;
  
  dfree(texture->internal_data, sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
  memset(texture, 0, sizeof(struct texture));
}
