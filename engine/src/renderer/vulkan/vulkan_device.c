#include "vulkan_device.h"

#include <stdint.h>
#include <string.h>

#include "core/logger.h"
#include "core/dstring.h"
#include "core/dmemory.h"

#define DEVICE_INVALID_INDEX UINT32_MAX
#define PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"

typedef struct vulkan_physical_device_requirements {
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    const char* const* device_extension_names;
    uint32_t device_extension_count;
    bool sampler_anisotropy;
    bool discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
    uint32_t graphics_family_index;
    uint32_t present_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;
} vulkan_physical_device_queue_family_info;

static bool select_physical_device(vulkan_context* context);
static bool physical_device_meets_requirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info,
    vulkan_swapchain_support_info* out_swapchain_support);

static void free_swapchain_support(vulkan_swapchain_support_info* support_info) {
    if (support_info->formats) {
        dfree(support_info->formats, sizeof(VkSurfaceFormatKHR) * support_info->format_count, MEMORY_TAG_RENDERER);
        support_info->formats = 0;
    }
    support_info->format_count = 0;

    if (support_info->present_modes) {
        dfree(support_info->present_modes, sizeof(VkPresentModeKHR) * support_info->present_mode_count, MEMORY_TAG_RENDERER);
        support_info->present_modes = 0;
    }
    support_info->present_mode_count = 0;
}

static bool device_supports_extension(VkPhysicalDevice device, const char* extension_name) {
    uint32_t available_extension_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));
    if (available_extension_count == 0) {
        return false;
    }

    VkExtensionProperties* available_extensions = (VkExtensionProperties*)dalloc(
        sizeof(VkExtensionProperties) * available_extension_count,
        MEMORY_TAG_RENDERER);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, available_extensions));

    bool found = false;
    for (uint32_t i = 0; i < available_extension_count; ++i) {
        if (strcmp(extension_name, available_extensions[i].extensionName) == 0) {
            found = true;
            break;
        }
    }

    dfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
    return found;
}

bool vulkan_device_create(vulkan_context* context) {
    if (!select_physical_device(context)) {
        return false;
    }

    DINFO("Creating logical device...");

    uint32_t requested_indices[3] = {
        (uint32_t)context->device.graphics_queue_index,
        (uint32_t)context->device.present_queue_index,
        (uint32_t)context->device.transfer_queue_index
    };

    uint32_t unique_indices[3];
    uint32_t index_count = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        bool already_added = false;
        for (uint32_t j = 0; j < index_count; ++j) {
            if (unique_indices[j] == requested_indices[i]) {
                already_added = true;
                break;
            }
        }
        if (!already_added) {
            unique_indices[index_count++] = requested_indices[i];
        }
    }

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[3];
    for (uint32_t i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].pNext = 0;
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].queueFamilyIndex = unique_indices[i];
        queue_create_infos[i].queueCount = 1;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures device_features = {0};
    device_features.samplerAnisotropy = VK_TRUE;

    const char* extension_names[2];
    uint32_t extension_count = 0;
    extension_names[extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    if (device_supports_extension(context->device.physical_device, PORTABILITY_SUBSET_EXTENSION_NAME)) {
        DINFO("Enabling extension: %s", PORTABILITY_SUBSET_EXTENSION_NAME);
        extension_names[extension_count++] = PORTABILITY_SUBSET_EXTENSION_NAME;
    }

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extension_names;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    VK_CHECK(vkCreateDevice(
        context->device.physical_device,
        &device_create_info,
        context->allocator,
        &context->device.logical_device));
    DINFO("Logical device created!");

    vkGetDeviceQueue(context->device.logical_device, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.present_queue_index, 0, &context->device.present_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.transfer_queue_index, 0, &context->device.transfer_queue);

    DINFO("Queues obtained");

    return true;
}

void vulkan_device_destroy(vulkan_context* context) {
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    DINFO("Destroying logical device...");
    if (context->device.logical_device) {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
    }

    DINFO("Releasing physical device resources...");
    context->device.physical_device = 0;

    free_swapchain_support(&context->device.swapchain_support);

    memset(&context->device.swapchain_support.capabilities, 0, sizeof(context->device.swapchain_support.capabilities));
    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    vulkan_swapchain_support_info* out_support_info) {
    free_swapchain_support(out_support_info);

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));
    if (out_support_info->format_count != 0) {
        out_support_info->formats = (VkSurfaceFormatKHR*)dalloc(
            sizeof(VkSurfaceFormatKHR) * out_support_info->format_count,
            MEMORY_TAG_RENDERER);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device,
            surface,
            &out_support_info->format_count,
            out_support_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, 0));
    if (out_support_info->present_mode_count != 0) {
        out_support_info->present_modes = (VkPresentModeKHR*)dalloc(
            sizeof(VkPresentModeKHR) * out_support_info->present_mode_count,
            MEMORY_TAG_RENDERER);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device,
            surface,
            &out_support_info->present_mode_count,
            out_support_info->present_modes));
    }
}

bool vulkan_device_detect_depth_format(vulkan_device* device) {
    const uint32_t candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    VkFormatFeatureFlags flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (uint32_t i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);
        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return true;
        }
    }
    return false;
}

static bool select_physical_device(vulkan_context* context) {
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
    if (physical_device_count == 0) {
        DFATAL("No devices which support Vulkan were found");
        return false;
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));

    const char* required_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vulkan_physical_device_requirements requirements = {0};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    requirements.sampler_anisotropy = true;
    requirements.discrete_gpu = false;
    requirements.device_extension_names = required_extensions;
    requirements.device_extension_count = (uint32_t)(sizeof(required_extensions) / sizeof(required_extensions[0]));

    bool found = false;
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        vulkan_physical_device_queue_family_info queue_info = {0};
        bool result = physical_device_meets_requirements(
            physical_devices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queue_info,
            &context->device.swapchain_support);
        if (!result) {
            continue;
        }

        DINFO("Selected device: '%s'", properties.deviceName);
        switch (properties.deviceType) {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                DINFO("GPU type is unknown");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                DINFO("GPU type is integrated");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                DINFO("GPU type is discrete");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                DINFO("GPU type is virtual");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                DINFO("GPU type is CPU");
                break;
        }

        DINFO("GPU driver version: %u.%u.%u",
            VK_VERSION_MAJOR(properties.driverVersion),
            VK_VERSION_MINOR(properties.driverVersion),
            VK_VERSION_PATCH(properties.driverVersion));
        DINFO("Vulkan API version: %u.%u.%u",
            VK_VERSION_MAJOR(properties.apiVersion),
            VK_VERSION_MINOR(properties.apiVersion),
            VK_VERSION_PATCH(properties.apiVersion));

        for (uint32_t j = 0; j < memory.memoryHeapCount; ++j) {
            float memory_size_gib = (((float)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
            if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                DINFO("Local GPU memory: %.2f GiB", memory_size_gib);
            } else {
                DINFO("Shared System memory: %.2f GiB", memory_size_gib);
            }
        }

        context->device.physical_device = physical_devices[i];
        context->device.graphics_queue_index = (int32_t)queue_info.graphics_family_index;
        context->device.present_queue_index = (int32_t)queue_info.present_family_index;
        context->device.transfer_queue_index = (int32_t)queue_info.transfer_family_index;
        context->device.properties = properties;
        context->device.features = features;
        context->device.memory = memory;
        found = true;
        break;
    }

    if (!found) {
        DERROR("No physical devices were found which meet the requirements");
        return false;
    }

    DINFO("Physical device selected");
    return true;
}

static bool physical_device_meets_requirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info,
    vulkan_swapchain_support_info* out_swapchain_support) {
    out_queue_info->graphics_family_index = DEVICE_INVALID_INDEX;
    out_queue_info->present_family_index = DEVICE_INVALID_INDEX;
    out_queue_info->compute_family_index = DEVICE_INVALID_INDEX;
    out_queue_info->transfer_family_index = DEVICE_INVALID_INDEX;

    if (requirements->discrete_gpu && properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        DINFO("Device is not a discrete gpu, and one is required. skipping.");
        return false;
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    if (queue_family_count == 0) {
        DINFO("Device has no queue families, skipping");
        return false;
    }
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    DINFO("Graphics | Present | Compute | Transfer | Name");
    uint8_t min_transfer_score = 255;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        uint8_t current_transfer_score = 0;

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            out_queue_info->graphics_family_index = i;
            ++current_transfer_score;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            out_queue_info->compute_family_index = i;
            ++current_transfer_score;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (current_transfer_score <= min_transfer_score) {
                min_transfer_score = current_transfer_score;
                out_queue_info->transfer_family_index = i;
            }
        }

        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
        if (supports_present) {
            out_queue_info->present_family_index = i;
        }
    }

    DINFO("         %u |            %u |            %u |            %u | %s",
        out_queue_info->graphics_family_index,
        out_queue_info->present_family_index,
        out_queue_info->compute_family_index,
        out_queue_info->transfer_family_index,
        properties->deviceName);

    bool queues_ok =
        (!requirements->graphics || out_queue_info->graphics_family_index != DEVICE_INVALID_INDEX) &&
        (!requirements->present || out_queue_info->present_family_index != DEVICE_INVALID_INDEX) &&
        (!requirements->compute || out_queue_info->compute_family_index != DEVICE_INVALID_INDEX) &&
        (!requirements->transfer || out_queue_info->transfer_family_index != DEVICE_INVALID_INDEX);
    if (!queues_ok) {
        DINFO("Device does not meet queue requirements, skipping");
        return false;
    }

    DINFO("Device meets queue requirements");
    DTRACE("Graphics family index: %u", out_queue_info->graphics_family_index);
    DTRACE("Present family index: %u", out_queue_info->present_family_index);
    DTRACE("Transfer family index: %u", out_queue_info->transfer_family_index);
    DTRACE("Compute family index: %u", out_queue_info->compute_family_index);

    if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
        DINFO("Device does not support samplerAnisotropy, skipping");
        return false;
    }

    for (uint32_t i = 0; i < requirements->device_extension_count; ++i) {
        if (!device_supports_extension(device, requirements->device_extension_names[i])) {
            DINFO("Required extension not found: '%s', skipping device", requirements->device_extension_names[i]);
            return false;
        }
    }

    vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);
    if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1) {
        free_swapchain_support(out_swapchain_support);
        DINFO("Required swapchain support not present, skipping device");
        return false;
    }

    return true;
}