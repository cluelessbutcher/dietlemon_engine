#include "vulkan_device.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "containers/darray.h"

#include <string.h>

typedef struct vulkan_physical_device_requirements {
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    const char** device_extension_names;
    bool sampler_anisotropy;
    bool discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
    uint32_t graphics_family_index;
    uint32_t present_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;
} vulkan_physical_device_queue_family_info;

bool select_physical_device(vulkan_context* context);
bool physical_device_meets_requirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_family_info,
    vulkan_swapchain_support_info* out_swapchain_support);

bool vulkan_device_create(vulkan_context* context) {
    if (!select_physical_device(context)) {
        return false;
    }

    return true;
}

void vulkan_device_destroy(vulkan_context* context) {
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vulkan_swapchain_support_info* out_support_info) {
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));

    if (out_support_info->format_count != 0) {
        if (!out_support_info->formats) {
            out_support_info->formats = dallocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, out_support_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, 0));
    if (out_support_info->present_mode_count != 0) {
        if (!out_support_info->present_modes) {
            out_support_info->present_modes = dallocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);
        }

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, out_support_info->present_modes));
    }
}

bool select_physical_device(vulkan_context* context) {
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
    if (physical_device_count == 0) {
        DFATAL("No device which support vulkan were found");
        return false;
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        vulkan_physical_device_requirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        requirements.sampler_anisotropy = true;
        requirements.discrete_gpu = true;
        requirements.device_extension_names = darray_create(const char*);
        darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        bool result = physical_device_meets_requirements(physical_devices[i],
                                                        context->surface,
                                                        &properties,
                                                        &features,
                                                        &requirements,
                                                        &queue_info,
                                                        &context->device.swapchain_support);

        if (result) {
            DINFO("Selected device: '%s'.", properties.deviceName);
            switch (properties.deviceType) {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                DINFO("GPU type is Unknown.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                DINFO("GPU type is Integrated.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                DINFO("GPU type is Descrete.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                DINFO("GPU type is Virtual.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                DINFO("GPU type is CPU.");
                break;
            }

            DINFO(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            DINFO(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            for (uint32_t j = 0; j < memory.memoryHeapCount; ++j) {
                float memory_size_gib = (((float)memory.memoryHeaps[j].size) / 1024.f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    DINFO("Local GPU memory: %.2f Gib", memory_size_gib);
                } else {
                    DINFO("Shared System memory: %.2f Gib", memory_size_gib);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;

            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }

    if (!context->device.physical_device) {
        DERROR("No physical devices were found which meet the requirments");
        return false;
    }

    DINFO("Physical device selected");
    return true;
}

bool physical_device_meets_requirements(VkPhysicalDevice device,
                                        VkSurfaceKHR surface,
                                        const VkPhysicalDeviceProperties* properties,
                                        const VkPhysicalDeviceFeatures* features,
                                        const vulkan_physical_device_requirements* requirements,
                                        vulkan_physical_device_queue_family_info* out_queue_info,
                                        vulkan_swapchain_support_info* out_swapchain_support) {

    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    if (requirements->discrete_gpu) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            DINFO("Device is not a discrete GPU and one is required, skipping...");
            return false;
        }
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
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

    DINFO("       %d |       %d |       %d |        %d | %s",
          out_queue_info->graphics_family_index != -1,
          out_queue_info->present_family_index != -1,
          out_queue_info->compute_family_index != -1,
          out_queue_info->transfer_family_index != -1,
          properties->deviceName);

    if (
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1))) {
        DINFO("Device meets queue requirements.");
        DTRACE("Graphics Family Index: %i", out_queue_info->graphics_family_index);
        DTRACE("Present Family Index:  %i", out_queue_info->present_family_index);
        DTRACE("Transfer Family Index: %i", out_queue_info->transfer_family_index);
        DTRACE("Compute Family Index:  %i", out_queue_info->compute_family_index);

        vulkan_device_query_swapchain_support(
            device,
            surface,
            out_swapchain_support);

        if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1) {
            if (out_swapchain_support->formats) {
                dfree(out_swapchain_support->formats, sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count, MEMORY_TAG_RENDERER);
            }
            if (out_swapchain_support->present_modes) {
                dfree(out_swapchain_support->present_modes, sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count, MEMORY_TAG_RENDERER);
            }
            DINFO("Required swapchain support not present, skipping device.");
            return false;
        }

        if (requirements->device_extension_names) {
            uint32_t available_extension_count = 0;
            VkExtensionProperties* available_extensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));
            if (available_extension_count != 0) {
                available_extensions = dallocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, available_extensions));
                uint32_t required_extension_count = darray_length(requirements->device_extension_names);
                for (uint32_t i = 0; i < required_extension_count; ++i) {
                    bool found = false;
                    for (uint32_t j = 0; j < available_extension_count; ++j) {
                        if (strcmp(requirements->device_extension_names[i], available_extensions[j].extensionName) == 0) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        DINFO("Required extension not found: '%s', skipping device.", requirements->device_extension_names[i]);
                        dfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }
            }
            dfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
        }

        if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
            DINFO("Device does not support samplerAnisotropy, skipping...");
            return false;
        }

        return true;
    }

    return false;
}
