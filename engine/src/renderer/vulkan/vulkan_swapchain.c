#include "vulkan_swapchain.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

void create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* out_swapchain) {
    create(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain) {
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain) {
    destroy(context, swapchain);
}

bool vulkan_swapchain_acquire_next_image_index(vulkan_context* context, vulkan_swapchain* swapchain, uint64_t timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, uint32_t* out_image_index) {
    VkResult result = vkAcquireNextImageKHR(context->device.logical_device, swapchain->handle, timeout_ns, image_available_semaphore, fence, out_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        DFATAL("Failed to Acquire swapchain image");
        return false;
    }

    return true;
}

void vulkan_swapchain_present(vulkan_context* context, vulkan_swapchain* swapchain, VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t present_image_index) {
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    } else if (result != VK_SUCCESS) {
        DFATAL("Failed to present swapchain image");
    }
}

void create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain) {
    VkExtent2D swapchain_extent = {width, height};
    bool found = false;
    for (uint32_t i = 0; i < context->device.swapchain_support.format_count; ++i) {
        VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->image_format = format;
            found = true;
            break;
        }
    }

    if (!found) {
        swapchain->image_format = context->device.swapchain_support.formats[0];
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < context->device.swapchain_support.present_mode_count; ++i) {
        VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    vulkan_device_query_swapchain_support(context->device.physical_device, context->surface, &context->device.swapchain_support);
    if (context->device.swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent = context->device.swapchain_support.capabilities.currentExtent;
    }

    VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;
    swapchain_extent.width = DCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = DCLAMP(swapchain_extent.height, min.height, max.height);

    uint32_t image_count = context->device.swapchain_support.capabilities.minImageCount + 1;
    if (context->device.swapchain_support.capabilities.maxImageCount > 0 && image_count > context->device.swapchain_support.capabilities.maxImageCount) {
        image_count = context->device.swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = swapchain->image_format.format;
    swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->device.graphics_queue_index != context->device.present_queue_index) {
        uint32_t queueFamilyIndices[] = {
            (uint32_t)context->device.graphics_queue_index,
            (uint32_t)context->device.present_queue_index};
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    swapchain_create_info.preTransform = context->device.swapchain_support.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_create_info, context->allocator, &swapchain->handle));

    context->current_frame = 0;

    swapchain->image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, 0));
    if (!swapchain->images) {
        swapchain->images = (VkImage*)dallocate(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->views) {
        swapchain->views = (VkImageView*)dallocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, swapchain->images));

    for (uint32_t i = 0; i < swapchain->image_count; ++i) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logical_device, &view_info, context->allocator, &swapchain->views[i]));
    }

    if (!vulkan_device_detect_depth_format(&context->device)) {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        DFATAL("Failed to find a supported format!");
    }

    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        swapchain_extent.width,
        swapchain_extent.height,
        context->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depth_attachment);
    
    DINFO("Swapchain created successfully.");
}

void destroy(vulkan_context* context, vulkan_swapchain* swapchain) {
    vulkan_image_destroy(context, &swapchain->depth_attachment);
    for (uint32_t i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(context->device.logical_device, swapchain->views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle, context->allocator);
}
