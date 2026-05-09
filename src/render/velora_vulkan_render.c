#include "defines.h"

#ifdef VULKAN_RENDER
#include "velora_render.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include <vulkan/vulkan.h>
#include "utils/vstring.h"
#include "vk_mem_alloc.h"
#include "utils/vmath.h"
#include "utils/vfile.h"
#include <vulkan/vk_enum_string_helper.h>
#include "core/event.h"
#include "utils/vimage.h"
#include "components/vrenderable.h"
#include "components/vcamera.h"
#include "components/vtransform.h"
#include "core/vecs.h"
#include "vmesh.h"
#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#elif VPLATFORM_LINUX
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

#define VELORA_VULKAN_API_VERSION VK_API_VERSION_1_4
#define MAX_FRAMES_IN_FLIGHT 2
//Currently can hold 100,000 vertices
#define VERTEX_BUFFER_SIZE (sizeof(vertex)*100000)
//Current can hold 1,000,000 indices
#define INDEX_BUFFER_SIZE (sizeof(u32)*1000000)

#define VK_CHECK(expr, msg)                           \
  {VkResult err = expr;                               \
  if(err != VK_SUCCESS){                              \
    VFATAL(msg);                                      \
    VFATAL("Vulkan Error: %s", string_VkResult(err)); \
    return FALSE;                                     \
  }}        

typedef struct _ubo{
  mat4x4 mvpMat;
  u64 textureIndex;
} ubo;

typedef struct _push_constant{
  u32 uboIndex;
  u32 padding;
  u64 padding2;
} push_constant;

typedef struct _vulkan_buffer{
  VkBuffer buffer;
  VmaAllocation memory;
  VmaAllocationInfo memory_info;
} vulkan_buffer;

typedef struct _vulkan_image{
  VkImage image;
  VmaAllocation memory;
  VmaAllocationInfo memory_info;
  VkImageView view;
  VkSampler sampler;
}vulkan_image;

typedef struct _vulkan_mesh{
  u64 vertexOffset;
  u64 indexOffset;
  u64 indexCount;
}vulkan_mesh;

typedef struct _vulkan_material{
  u64 baseColorTextureIndex;
}vulkan_material;

typedef struct _swapchain_support{
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VkSurfaceFormatKHR* surfaceFormats;
  u32 surfaceFormatCount;
  VkPresentModeKHR* presentModes;
  u32 presentModeCount;
} swapchain_support;

typedef struct _vulkan_state{
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkPhysicalDeviceProperties physicalDeviceProps;
  VkDevice logicalDevice;
  u32 graphicsQueueIndex;
  u32 presentQueueIndex;
  u32 transferQueueIndex;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue transferQueue;
  VmaAllocator allocator;
  VkSurfaceKHR surface;
  swapchain_support swapchainSupportDetails;
  VkFormat swapchainFormat;
  VkImage* swapchainImages;
  u32 swapchainImageCount;
  VkExtent2D swapchainExtent;
  VkSwapchainKHR swapchain;
  VkImageView* swapchainImageViews;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkFramebuffer* frameBuffers;
  VkCommandPool graphicsCommandPool;
  VkCommandBuffer* commandBuffer;
  VkCommandPool transferCommandPool;
  VkSemaphore* imageAvailable, *renderFinished;
  VkFence* inFlight;
  u32 currentFrame;
  b8 windowResized, windowMinimized;
  u32 newWidth, newHeight;
  vulkan_buffer vertexIndexBuffer;
  vulkan_buffer uniformBuffer;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet* descriptorSets;
  vulkan_image depthImage;
  vulkan_image textures[VELORA_MAX_TEXTURES];
  vulkan_image defaultTexture;
  u64 curTextureIndex;
  vulkan_mesh meshes[VELORA_MAX_MESHES];
  vulkan_material materials[VELORA_MAX_MATERIALS];
  u64 curMeshIndex;
  u64 vertexBufferUsedSize;
  u64 indexBufferUsedSize;
  #ifdef _DEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
  #endif
} vulkan_state;

#ifdef _DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData){
    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
      VDEBUG(pCallbackData->pMessage);
    }else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
      VINFO(pCallbackData->pMessage);
    }else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
      VWARN(pCallbackData->pMessage);
    }else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
      VERROR(pCallbackData->pMessage);
    }
    return VK_FALSE;
  }

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
      func(instance, debugMessenger, pAllocator);
  }
}

void populate_debug_create_info(VkDebugUtilsMessengerCreateInfoEXT* createInfo){
  createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo->messageSeverity = 
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo->messageType = 
  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo->pfnUserCallback = debugCallback;
  createInfo->pNext = NULL;
}

b8 initiate_validation_callback(vulkan_state* state){
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  populate_debug_create_info(&createInfo);
  VK_CHECK(
    CreateDebugUtilsMessengerEXT(state->instance, &createInfo, NULL, &(state->debugMessenger)),
    "Unable to create the validation layer callback"
  );
  return TRUE;
}
#endif

b8 check_layer_support(const char** validation_layers, u32 num_of_layers){
  u32 layerCount;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, NULL), "Unable to enumerate validation layers");

  VkLayerProperties availableLayers[layerCount];
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers), "Unable to fill an array with validation layers available");
  for(int i = 0; i < num_of_layers; i++){
    u8 layerFound = FALSE;
    for(int j = 0; j < layerCount; j++){
      if(vstrcmp(validation_layers[i], availableLayers[j].layerName) == TRUE){
        layerFound = TRUE;
        break;
      }
    }
    if(layerFound == FALSE){
      return FALSE;
    }
  }
  return TRUE;
}

void enable_optional_feature(const char** enabled_layers, u32* current_count, const char* new_layer){
  enabled_layers[(*current_count)] = new_layer;
  (*current_count)++;
}

b8 create_vulkan_instance(vulkan_state* state, const char* app_name){
  const char* enabled_layers[10]; //10 should be enough for now
  u32 num_of_layers = 0;
  const char* extensions[10]; //10 should be enough here too
  u32 extension_count = 0;
  #ifdef _DEBUG
  #define VK_KHRONOS_VALIDATION_VALIDATE_SYNC TRUE
  enable_optional_feature(enabled_layers, &num_of_layers, "VK_LAYER_KHRONOS_validation");
  enable_optional_feature(extensions, &extension_count, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  #endif
  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = app_name,
    .applicationVersion = VK_MAKE_API_VERSION(0,0,1,0),
    .pEngineName = "Velora",
    .engineVersion = VK_MAKE_API_VERSION(0,0,1,0),
    .apiVersion = VELORA_VULKAN_API_VERSION,
  };
  #ifdef VPLATFORM_WINDOWS
  enable_optional_feature(extensions, &extension_count, VK_KHR_SURFACE_EXTENSION_NAME);
  enable_optional_feature(extensions, &extension_count, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
  #elif VPLATFORM_LINUX
  enable_optional_feature(extensions, &extension_count, VK_KHR_SURFACE_EXTENSION_NAME);
  enable_optional_feature(extensions, &extension_count, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
  #else
  VFATAL("Vulkan render backend chosen on unsupported system");
  return FALSE;
  #endif
  //enable_optional_feature(extensions, &extension_count, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  if(check_layer_support(enabled_layers, num_of_layers) == FALSE){
    VFATAL("At least one enabled Vulkan layer was not supported on this system");
    return FALSE;
  }
  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo,
    .ppEnabledExtensionNames = extensions,
    .enabledExtensionCount = extension_count,
    .ppEnabledLayerNames = enabled_layers,
    .enabledLayerCount = num_of_layers,
  };
  #ifdef _DEBUG
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfoInstance = {};
  populate_debug_create_info(&debugCreateInfoInstance);
  createInfo.pNext= (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfoInstance;
  #endif
  VK_CHECK(vkCreateInstance(&createInfo, NULL, &state->instance), "Unable to start Vulkan instance");
  #ifdef _DEBUG
  if(initiate_validation_callback(state) == FALSE){
    return FALSE;
  }
  #endif
  return TRUE;
}

b8 has_stencil_comp(VkFormat format){
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer begin_single_command(vulkan_state* state, VkCommandPool commandPool){
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = commandPool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(state->logicalDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void end_single_command(vulkan_state* state, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer){
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
  };
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(state->logicalDevice, commandPool, 1, &commandBuffer);
}

b8 transition_image_layout(vulkan_state* state, vulkan_image* image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
  VkCommandBuffer commandBuffer = begin_single_command(
    state,
    state->graphicsCommandPool
  );
  VkAccessFlags srcAccessMask = 0;
  VkAccessFlags dstAccessMask = 0;

  VkPipelineStageFlags srcStage = 0;
  VkPipelineStageFlags dstStage = 0;

  VkImageAspectFlags aspectFlag = 0;

  if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    if(has_stencil_comp(format)){
      aspectFlag |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }else{
    aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED){
    if(newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
      srcAccessMask = 0;
      dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }else if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
      srcAccessMask = 0;
      dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
  }else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
    srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }else{
    VERROR("Unsupported layout transition");
    return FALSE;
  }

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image->image,
    .subresourceRange.aspectMask = aspectFlag,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .srcAccessMask = srcAccessMask,
    .dstAccessMask = dstAccessMask,
  };
  vkCmdPipelineBarrier(
    commandBuffer,
    srcStage, dstStage,
    0,
    0, VK_NULL_HANDLE,
    0, VK_NULL_HANDLE,
    1, &barrier
  );

  end_single_command(
    state,
    state->graphicsCommandPool,
    state->graphicsQueue,
    commandBuffer
  );
  return TRUE;
}

b8 create_image_view(vulkan_state* state, VkImage image, VkFormat format, VkImageView* outView, VkImageAspectFlags aspectFlags){
  VkComponentMapping compMapping = {
    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
  };
  VkImageSubresourceRange subresRange = {
    .aspectMask = aspectFlags,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1
  };
  VkImageViewCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .components = compMapping,
    .subresourceRange = subresRange,
  };
  VK_CHECK(vkCreateImageView(
    state->logicalDevice,
    &createInfo,
    NULL,
    outView
  ), "Unable to create image view");
  return TRUE;
}

b8 create_exclusive_image(
  vulkan_state *state, 
  vulkan_image* image,
  u32 width,
  u32 height,
  VkFormat format,
  VkImageUsageFlags flags,
  VmaAllocationCreateFlags vmaFlags
){
  VkImageCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = width,
    .extent.height = height,
    .extent.depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage = flags,//VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .flags = 0,
  };
  VmaAllocationCreateInfo allocInfo = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = vmaFlags,
    .priority = 1.0f,
  };
  VK_CHECK(vmaCreateImage(
    state->allocator,
    &createInfo,
    &allocInfo,
    &image->image,
    &image->memory,
    &image->memory_info
  ), "Unable to create image");
  return TRUE;
}

b8 create_shared_image(
  vulkan_state* state,
  vulkan_image* image,
  u32 width,
  u32 height,
  VkFormat format,
  VkImageUsageFlags flags,
  VmaAllocationCreateFlags vmaFlags,
  u32* queueFamilyIndices,
  u32 queueFamilyIndiciesCount
){
  VkImageCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = width,
    .extent.height = height,
    .extent.depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage = flags,//VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_CONCURRENT,
    .pQueueFamilyIndices = queueFamilyIndices,
    .queueFamilyIndexCount = queueFamilyIndiciesCount,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .flags = 0,
  };
  VmaAllocationCreateInfo allocInfo = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = vmaFlags,
    .priority = 1.0f,
  };
  VK_CHECK(vmaCreateImage(
    state->allocator,
    &createInfo,
    &allocInfo,
    &image->image,
    &image->memory,
    &image->memory_info
  ), "Unable to create image");
  return TRUE;
}

b8 find_supported_format(vulkan_state* state, const VkFormat* candidates, u32 candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat* outFormat){
  for(int i = 0; i < candidateCount; i++){
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(state->physicalDevice, candidates[i], &props);
    if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features){
      (*outFormat) = candidates[i];
      return TRUE;
    }else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
      (*outFormat) = candidates[i];
      return TRUE;
    }
  }
  return FALSE;
}

b8 find_depth_format(vulkan_state* state, VkFormat* outFormat){
  const u32 formatCount = 3;
  const VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  return find_supported_format(
    state,
    formats,
    formatCount,
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
    outFormat
  );
}

b8 create_depth_resources(vulkan_state* state){
  VkFormat depthFormat = {0};
  VEL_CHECK(find_depth_format(state, &depthFormat));
  VEL_CHECK(create_exclusive_image(
    state,
    &state->depthImage,
    state->swapchainExtent.width,
    state->swapchainExtent.height,
    depthFormat,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
  ));
  VEL_CHECK(create_image_view(
    state,
    state->depthImage.image,
    depthFormat,
    &state->depthImage.view,
    VK_IMAGE_ASPECT_DEPTH_BIT
  ));
  VEL_CHECK(transition_image_layout(
    state,
    &state->depthImage,
    depthFormat,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  ));
  return TRUE;
}

b8 obtain_swapchain_info(vulkan_state* state, VkPhysicalDevice device){
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    device, 
    state->surface, 
    &state->swapchainSupportDetails.surfaceCapabilities
  ), "Unable to get surface capabilities of the chosen phsyical device");
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
    device, 
    state->surface, 
    &state->swapchainSupportDetails.surfaceFormatCount, 
    NULL
  ), "Unabled to enumerate surface formats for chosen device");
  if(state->swapchainSupportDetails.surfaceFormatCount == 0){
    VFATAL("No surface formats for the chosen physical device");
    return FALSE;
  }
  state->swapchainSupportDetails.surfaceFormats = vallocate(
    sizeof(VkSurfaceFormatKHR)*(state->swapchainSupportDetails.surfaceFormatCount), 
    MEMORY_TAG_RENDERER
  );
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
    device, 
    state->surface, 
    &state->swapchainSupportDetails.surfaceFormatCount, 
    state->swapchainSupportDetails.surfaceFormats
  ), "Unable to fill array with surface formats for chosen physical device");
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
    device,
    state->surface,
    &state->swapchainSupportDetails.presentModeCount,
    NULL
  ), "Unable to enumerate present modes for chosen physical device");
  if(state->swapchainSupportDetails.presentModeCount == 0){
    VFATAL("No present modes for this graphics card");
    return FALSE;
  }
  state->swapchainSupportDetails.presentModes = vallocate(
    sizeof(VkPresentModeKHR)*state->swapchainSupportDetails.presentModeCount, 
    MEMORY_TAG_RENDERER
  );
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
    device,
    state->surface,
    &state->swapchainSupportDetails.presentModeCount,
    state->swapchainSupportDetails.presentModes
  ), "Unable to fill array with present modes for chosen physical device");
  return TRUE;
}

b8 is_physical_device_suitable(vulkan_state* state, VkPhysicalDevice device, const char** extensions, u32 extensionCount){
  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  if(queueFamilyCount == 0){
    VFATAL("Chosen physical device has no queue families");
    return FALSE;
  }
  VkQueueFamilyProperties props[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, props);

  u8 graphicsQueueObtained = FALSE;
  u8 transferQueueObtained = FALSE;
  u8 presentQueueObtained = FALSE;
  VkBool32 isPresentCapable;

  for(int i = 0; i < queueFamilyCount; i++){
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, state->surface, &isPresentCapable), "Unable to check if this device is present capable");
    if(graphicsQueueObtained == FALSE && props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
      state->graphicsQueueIndex = i;
      graphicsQueueObtained = TRUE;
    }
    if(transferQueueObtained == FALSE && props[i].queueFlags & VK_QUEUE_TRANSFER_BIT && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0){
      state->transferQueueIndex = i;
      transferQueueObtained = TRUE;
    }
    if(presentQueueObtained == FALSE && isPresentCapable){
      state->presentQueueIndex = i;
      presentQueueObtained = TRUE;
    }
  }

  // No dedicated transfer queue exists, luckily graphics queues are implicitly transfer queues so just make those the same
  if(transferQueueObtained == FALSE && graphicsQueueObtained == TRUE){
    state->transferQueueIndex = state->graphicsQueueIndex;
    transferQueueObtained = TRUE;
  }

  u32 deviceExtensionCount = 0;
  VK_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, NULL), "Unable to enumerate extentions on physical device");
  if(deviceExtensionCount == 0){
    return FALSE; // We need at least the swapchain extension
  }

  VkExtensionProperties exProps[deviceExtensionCount];
  VK_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, exProps), "Unable to fill array with device extensions");
  u32 foundExtensions = 0;
  for(int i = 0; i < extensionCount; i++){
    for(int j = 0; j < deviceExtensionCount; j++){
      if(vstrcmp(extensions[i], exProps[j].extensionName) == TRUE){
        foundExtensions++;
        break;//extension has been found, no need to keep looking
      }
    }
  }

  u8 swapchainSuitable = obtain_swapchain_info(state, device);

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return (swapchainSuitable && graphicsQueueObtained && transferQueueObtained && presentQueueObtained && (foundExtensions == extensionCount) && supportedFeatures.samplerAnisotropy);
}

b8 obtain_physical_device(vulkan_state* state, const char** extensions, u32 extensionCount){
  u32 deviceCount = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(state->instance, &deviceCount, NULL), "Unable to enumerate physical devices");
  if(deviceCount == 0){
    VFATAL("No GPU capable of supporting a vulkan renderer was found");
    return FALSE;
  }
  VkPhysicalDevice devices[deviceCount];
  VK_CHECK(vkEnumeratePhysicalDevices(state->instance, &deviceCount, devices), "Unable to fill array with phsyical devices");
  for(int i = 0; i < deviceCount; deviceCount++){
    if(is_physical_device_suitable(state, devices[i], extensions, extensionCount)){
      state->physicalDevice = devices[i];
      return TRUE;
    }
  }
  return FALSE;
}

void activate_queue(VkDeviceQueueCreateInfo* queueArray, u32* current_count, u32 queueFamilyIndex, u32 queueCount, float* queuePriories){
  u32 i = (*current_count);
  vzero_memory(&queueArray[i], sizeof(VkDeviceQueueCreateInfo));
  queueArray[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueArray[i].queueFamilyIndex = queueFamilyIndex;
  queueArray[i].queueCount = queueCount;
  queueArray[i].pQueuePriorities = queuePriories;
  (*current_count)++;
}

b8 create_logical_device(vulkan_state* state, const char** extensions, u32 extensionCount){
  float queuePri = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfos[10];
  u32 queueCount = 0;
  activate_queue(queueCreateInfos, &queueCount, state->graphicsQueueIndex, 1, &queuePri);
  if(state->graphicsQueueIndex != state->presentQueueIndex){
    activate_queue(queueCreateInfos, &queueCount, state->presentQueueIndex, 1, &queuePri);
  }
  if(state->graphicsQueueIndex != state->transferQueueIndex){
    activate_queue(queueCreateInfos, &queueCount, state->transferQueueIndex, 1, &queuePri);
  }
  VkPhysicalDeviceFeatures deviceFeatues = {0};
  deviceFeatues.samplerAnisotropy = VK_TRUE;
  VkDeviceCreateInfo logicalDeviceCreateInfo ={
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = queueCreateInfos,
    .queueCreateInfoCount = queueCount,
    .pEnabledFeatures = &deviceFeatues,
    .ppEnabledExtensionNames = extensions,
    .enabledExtensionCount = extensionCount,
    .enabledLayerCount = 0, //We would need to specify the validation layers in this if it was an older implementation of Vulkan, I can't care though
  };
  VK_CHECK(
    vkCreateDevice(
      state->physicalDevice, 
      &logicalDeviceCreateInfo, 
      NULL, 
      &state->logicalDevice
    ), 
    "Unable to create logical device"
  );
  vkGetDeviceQueue(state->logicalDevice, state->graphicsQueueIndex, 0, &state->graphicsQueue);
  vkGetDeviceQueue(state->logicalDevice, state->presentQueueIndex, 0, &state->presentQueue);
  vkGetDeviceQueue(state->logicalDevice, state->transferQueueIndex, 0, &state->transferQueue);
  vkGetPhysicalDeviceProperties(state->physicalDevice, &state->physicalDeviceProps);
  return TRUE;
}

b8 create_vma_allocator(vulkan_state* state){
  VmaAllocatorCreateInfo createInfo = {
    .device = state->logicalDevice,
    .physicalDevice = state->physicalDevice,
    .instance = state->instance,
    .vulkanApiVersion = VELORA_VULKAN_API_VERSION,
  };
  VK_CHECK(vmaCreateAllocator(&createInfo, &state->allocator), "Unable to create VMA allocator");
  return TRUE;
}

b8 choose_surface_format(vulkan_state* state, VkSurfaceFormatKHR *out_format){
  for(int i = 0; i < state->swapchainSupportDetails.surfaceFormatCount; i++){
    if(state->swapchainSupportDetails.surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
      && state->swapchainSupportDetails.surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB){
        (*out_format) = state->swapchainSupportDetails.surfaceFormats[i];
        return TRUE;
    }
  }
  VFATAL("No suitable surface formats found");
  return FALSE;
}

b8 choose_present_mode(vulkan_state* state, VkPresentModeKHR* out_mode){
  for(int i = 0; i < state->swapchainSupportDetails.presentModeCount; i++){
    if(state->swapchainSupportDetails.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
      (*out_mode) = VK_PRESENT_MODE_MAILBOX_KHR;
      return TRUE;
    }
  }
  // FIFO is always there, we can remove this if we require only mailbox for some reason.
  (*out_mode) = VK_PRESENT_MODE_FIFO_KHR;
  return TRUE;
}

VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR caps, u32 width, u32 height){
  if(caps.currentExtent.width != U32_MAX){
    return caps.currentExtent;
  }
  VkExtent2D extent = {
    .height = vclamp(height, caps.minImageExtent.height, caps.maxImageExtent.height),
    .width = vclamp(width, caps.minImageExtent.width, caps.maxImageExtent.width),
  };
  return extent;
}

b8 create_swapchain(vulkan_state* state, u32 width, u32 height){
  VkSurfaceFormatKHR surfaceFormat;
  VEL_CHECK(choose_surface_format(state, &surfaceFormat));
  state->swapchainFormat = surfaceFormat.format;
  VkPresentModeKHR presentMode;
  VEL_CHECK(choose_present_mode(state, &presentMode));
  state->swapchainExtent = choose_swapchain_extent(state->swapchainSupportDetails.surfaceCapabilities, width, height);
  u32 imageCount = state->swapchainSupportDetails.surfaceCapabilities.minImageCount+1;
  if(
    state->swapchainSupportDetails.surfaceCapabilities.maxImageCount > 0 && 
    imageCount > state->swapchainSupportDetails.surfaceCapabilities.maxImageCount
  ){
    imageCount = state->swapchainSupportDetails.surfaceCapabilities.maxImageCount;
  }
  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = state->surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .presentMode = presentMode,
    .imageExtent = state->swapchainExtent,
    .imageArrayLayers = 1, // The only reason this would be more than one is for streoscopic 3d games
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = state->swapchainSupportDetails.surfaceCapabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
  if(state->graphicsQueueIndex != state->presentQueueIndex){
    u32 indices[2];
    indices[0] = state->graphicsQueueIndex;
    indices[1] = state->presentQueueIndex;
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = indices;
  }else{
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional, not accessed in exclusive mode
    createInfo.pQueueFamilyIndices = VK_NULL_HANDLE; // Optional, not accessed in exclusive mode
  }
  VK_CHECK(
    vkCreateSwapchainKHR(state->logicalDevice, &createInfo, NULL, &state->swapchain), 
    "Unable to create swapchain"
  );
  vkGetSwapchainImagesKHR(state->logicalDevice, state->swapchain, &state->swapchainImageCount, NULL);
  state->swapchainImages = vallocate(sizeof(VkImage)*state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vkGetSwapchainImagesKHR(state->logicalDevice, state->swapchain, &state->swapchainImageCount, state->swapchainImages);
  return TRUE;
}

b8 create_swapchain_image_views(vulkan_state* state){
  state->swapchainImageViews = vallocate(sizeof(VkImageView)*state->swapchainImageCount, MEMORY_TAG_RENDERER);
  for(int i = 0; i < state->swapchainImageCount; i++){
    VEL_CHECK(
      create_image_view(
        state, 
        state->swapchainImages[i],
        state->swapchainFormat, 
        &state->swapchainImageViews[i],
        VK_IMAGE_ASPECT_COLOR_BIT
      )
    );
  }
  return TRUE;
}

void destroy_swapchain(vulkan_state* vk_state){
  for(int i = 0; i< vk_state->swapchainImageCount; i++){
    vkDestroyFramebuffer(vk_state->logicalDevice, vk_state->frameBuffers[i], NULL);
  }
  for(int i = 0; i < vk_state->swapchainImageCount; i++){
    vkDestroyImageView(vk_state->logicalDevice, vk_state->swapchainImageViews[i], NULL);
  }
  vfree(vk_state->frameBuffers, sizeof(VkFramebuffer)*vk_state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vfree(vk_state->swapchainImages, sizeof(VkImage)*vk_state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vfree(vk_state->swapchainImageViews, sizeof(VkImageView)*vk_state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vkDestroySwapchainKHR(vk_state->logicalDevice, vk_state->swapchain, NULL);
}

VkShaderModule get_shader_module(vulkan_state* state, const char* shaderFileName){
  velora_file shaderFile = {0};
  VEL_CHECK(get_file_contents(shaderFileName, &shaderFile));
  VkShaderModuleCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shaderFile.size,
    .pCode = (u32*)shaderFile.contents
  };
  VkShaderModule ret_mod;
  VK_CHECK(vkCreateShaderModule(state->logicalDevice, &createInfo, NULL, &ret_mod), "Failed to create shader module");
  free_velora_file(&shaderFile);
  return ret_mod;
}

b8 create_render_pass(vulkan_state* state){
  VkAttachmentDescription colorAttachment = {
    .format = state->swapchainFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT, // This will change if multi sampling is enabled
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // What to do with the attachment (the swapchain image), Clear clears it out
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Store makes the render results stored so we can read it later
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // We disabled stencil buffer stuff so this doesn't matter
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // We don't care about maintainining the image incoming, we're going to clear it anyway
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // We're going to render it into the window so it needs to be ready
  };
  VkFormat depthFormat;
  VEL_CHECK(find_depth_format(state, &depthFormat));
  VkAttachmentDescription depthAttachment = {
    .format = depthFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, 
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  VkAttachmentReference colorAttachmentRef = { // This is the layout(location = 0) out vec4 outColor in the fragment shader
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkAttachmentReference depthAttachmentRef = { // This is the layout(location = 0) out vec4 outColor in the fragment shader
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentRef,
    .pDepthStencilAttachment = &depthAttachmentRef,
  };
  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL, // This refers to the implicit subpass at the beginning of the pipeline
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };
  VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 2,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };
  VK_CHECK(vkCreateRenderPass(state->logicalDevice, &createInfo, NULL, &state->renderPass), "Unable to create render pass");
  return TRUE;
}

b8 create_graphics_pipeline(vulkan_state* state){
  VkShaderModule vertMod = get_shader_module(state, "vert.spv");
  VkShaderModule fragMod = get_shader_module(state, "frag.spv");
  VkPipelineShaderStageCreateInfo vertexCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertMod,
    .pName = "main", // The name of the entry point function in the GLSL script
    .pSpecializationInfo = NULL,
  };
  VkPipelineShaderStageCreateInfo fragmentCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragMod,
    .pName = "main", // The name of the entry point function in the GLSL script
    .pSpecializationInfo = NULL,
  };
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertexCreateInfo, fragmentCreateInfo};
  u32 dynamicStateCount = 2;
  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = dynamicStateCount,
    .pDynamicStates = dynamicStates,
  };
  VkVertexInputBindingDescription bindingDesc = {
    .binding = 0,
    .stride = sizeof(vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  const u8 numOfVertexStructMembers = 3;
  VkVertexInputAttributeDescription vertexStructDesc[numOfVertexStructMembers];

  vertexStructDesc[0].binding = 0;
  vertexStructDesc[0].location = 0;
  vertexStructDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexStructDesc[0].offset = offsetof(vertex, pos);

  vertexStructDesc[1].binding = 0;
  vertexStructDesc[1].location = 1;
  vertexStructDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexStructDesc[1].offset = offsetof(vertex, normal);

  vertexStructDesc[2].binding = 0;
  vertexStructDesc[2].location = 2;
  vertexStructDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
  vertexStructDesc[2].offset = offsetof(vertex, texCoord);
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindingDesc,
    .vertexAttributeDescriptionCount = numOfVertexStructMembers,
    .pVertexAttributeDescriptions = vertexStructDesc,
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)state->swapchainExtent.width,
    .height = (float)state->swapchainExtent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  VkRect2D scissor = {
    .offset = {
      .x = 0,
      .y = 0
    },
    .extent = state->swapchainExtent
  };
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };
  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL, // The other modes are good for wireframes and vertex points but requires enabling a GPU feature in the logical device
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f, //Optional when depthBias is set to false
    .depthBiasClamp = 0.0f, //Optional when depthBias is set to false
    .depthBiasSlopeFactor = 0.0f, //Optional when depthBias is set to false
  };
  VkPipelineMultisampleStateCreateInfo multiSampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE, // This disables multisampling. The rest of the values are optional when this is false.
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
    .colorWriteMask = 
      VK_COLOR_COMPONENT_R_BIT |
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT, // This is bitwise AND'd against the combined colors, how they're combined is described below
    .blendEnable = VK_FALSE, // This disables colorblending, the rest of the values are optional while this is false
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Factor to multiply the src color
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Factor to mutiply the dst color
    .colorBlendOp = VK_BLEND_OP_ADD, // How to combine the src and dst color, this case add them
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
  };
  VkPipelineColorBlendStateCreateInfo colorBlending = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE, //This overrides blend enable if true
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f,
  };
  VkPipelineDepthStencilStateCreateInfo depthStencil = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
    .stencilTestEnable = VK_FALSE,
    .front = {0},
    .back = {0},
  };
  if(sizeof(push_constant) % 16 != 0){
    VFATAL("Push constants need to be aligned to 16 bytes.");
    VFATAL("Current size of push constant struct: %d", sizeof(push_constant));
    return FALSE;
  }
  VkPushConstantRange pushRange = {
    .offset = 0,
    .size = sizeof(push_constant),
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
  };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &state->descriptorSetLayout,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushRange,
  };
  VK_CHECK(
    vkCreatePipelineLayout(
      state->logicalDevice, 
      &pipelineLayoutInfo, 
      NULL, 
      &state->pipelineLayout
    ), 
    "Unable to create pipeline layout"
  );
  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shaderStages, // Programmable shader stages, this time it's just vertex and fragment
    .pVertexInputState = &vertexInputCreateInfo,
    .pInputAssemblyState = &inputAssemblyCreateInfo,
    .pViewportState = &viewportStateCreateInfo,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multiSampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicStateCreateInfo,
    .layout = state->pipelineLayout,
    .renderPass = state->renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };
  VK_CHECK(
    vkCreateGraphicsPipelines(
      state->logicalDevice, 
      VK_NULL_HANDLE, 
      1, 
      &pipelineCreateInfo, 
      NULL, 
      &state->graphicsPipeline
    ), 
    "Unable to create graphics pipeline"
  );
  vkDestroyShaderModule(state->logicalDevice, vertMod, NULL);
  vkDestroyShaderModule(state->logicalDevice, fragMod, NULL);
  return TRUE;
}

b8 create_frame_buffers(vulkan_state* state){
  state->frameBuffers = vallocate(
    sizeof(VkFramebuffer)*state->swapchainImageCount, 
    MEMORY_TAG_RENDERER
  );
  for(int i = 0; i < state->swapchainImageCount; i++){
    VkImageView attachments[] = {
      state->swapchainImageViews[i],
      state->depthImage.view
    };
    VkFramebufferCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = state->renderPass,
      .attachmentCount = 2,
      .pAttachments = attachments,
      .width = state->swapchainExtent.width,
      .height = state->swapchainExtent.height,
      .layers = 1,
    };
    VK_CHECK(
      vkCreateFramebuffer(
        state->logicalDevice, 
        &createInfo, 
        NULL, 
        &state->frameBuffers[i]
      ), 
      "Unable to create framebuffer"
    );
  }
  return TRUE;
}

b8 create_command_pool(vulkan_state* state){
  VkCommandPoolCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = state->graphicsQueueIndex,
  };
  VK_CHECK(
    vkCreateCommandPool(
      state->logicalDevice,
      &createInfo,
      NULL,
      &state->graphicsCommandPool
    ), 
    "Unable to create command pool"
  );
  VkCommandPoolCreateInfo transferInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = state->transferQueueIndex,
  };
  VK_CHECK(
    vkCreateCommandPool(
      state->logicalDevice,
      &transferInfo,
      NULL,
      &state->transferCommandPool
    ),
    "Unable to create transfer command pool"
  );
  return TRUE;
}

b8 create_command_buffer(vulkan_state* state){
  state->commandBuffer = vallocate(sizeof(VkCommandBuffer)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = state->graphicsCommandPool,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  };
  VK_CHECK(
    vkAllocateCommandBuffers(
      state->logicalDevice, 
      &allocInfo, 
      state->commandBuffer
    ), 
    "Unable to allocate command buffers"
  );
  return TRUE;
}

b8 create_sync_objects(vulkan_state* state){
  state->imageAvailable = vallocate(sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  state->renderFinished = vallocate(sizeof(VkSemaphore)*state->swapchainImageCount, MEMORY_TAG_RENDERER);
  state->inFlight = vallocate(sizeof(VkFence)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  VkSemaphoreCreateInfo semInfo = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceInfo = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT, // Make it so that the render loop doesn't hang on the fence for the first frame
  };
  for(int i = 0; i < state->swapchainImageCount; i++){
    VK_CHECK(
      vkCreateSemaphore(state->logicalDevice, &semInfo, NULL, &state->renderFinished[i]),
      "Unable to create Semaphore for renderFinished"
    );
  }
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    VK_CHECK(
      vkCreateSemaphore(state->logicalDevice, &semInfo, NULL, &state->imageAvailable[i]),
      "Unable to create Semaphore for imageAvailable"
    );
    VK_CHECK(
      vkCreateFence(state->logicalDevice, &fenceInfo, NULL, &state->inFlight[i]),
      "Unable to create fence"
    );
  }
  return TRUE;
}

void destroy_sync_objects(vulkan_state* state){
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    vkDestroySemaphore(state->logicalDevice, state->imageAvailable[i], NULL);
    vkDestroyFence(state->logicalDevice, state->inFlight[i], NULL);
  }
  for(int i = 0; i < state->swapchainImageCount; i++){
    vkDestroySemaphore(state->logicalDevice, state->renderFinished[i], NULL);
  }
  vfree(state->imageAvailable, sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vfree(state->renderFinished, sizeof(VkSemaphore)*state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vfree(state->inFlight, sizeof(VkFence)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
}

void destroy_vulkan_buffer(vulkan_state* state, vulkan_buffer* buffer){
  vmaDestroyBuffer(
    state->allocator, 
    buffer->buffer,
    buffer->memory
  );
}

void destroy_vulkan_image(vulkan_state* state, vulkan_image* image){
  vkDestroySampler(
    state->logicalDevice,
    image->sampler,
    VK_NULL_HANDLE
  );
  vkDestroyImageView(
    state->logicalDevice,
    image->view,
    VK_NULL_HANDLE
  );
  vmaDestroyImage(
    state->allocator,
    image->image,
    image->memory
  );
}

b8 recreate_swapchain(vulkan_state* state, u32 width, u32 height){
  vkDeviceWaitIdle(state->logicalDevice);

  destroy_vulkan_image(state, &state->depthImage);
  destroy_swapchain(state);
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    state->physicalDevice,
    state->surface, 
    &state->swapchainSupportDetails.surfaceCapabilities
  ), "Unable to get surface capabilities of the chosen phsyical device");
  VEL_CHECK(create_swapchain(state, width, height));
  VEL_CHECK(create_swapchain_image_views(state));
  VEL_CHECK(create_depth_resources(state));
  VEL_CHECK(create_frame_buffers(state));
  return TRUE;
}

b8 resize_handler(event* newEvent, void* state){
  if(newEvent->event_type == ENGINE_WINDOW_RESIZE){
    resize_data* eventData = (resize_data*)newEvent->event_data;
    render_state* ren_state = (render_state*)state;
    vulkan_state* state = (vulkan_state*)ren_state->internal_render_state;
    state->newHeight = eventData->height;
    state->newWidth = eventData->width;
    state->windowResized = TRUE;
  }
  return TRUE;
}


b8 create_shared_buffer(
  vulkan_state *state, 
  vulkan_buffer* buffer, 
  VkBufferUsageFlagBits bufferUsage, 
  VkDeviceSize bufferSize, 
  VkMemoryPropertyFlags memoryType,
  VmaAllocationCreateFlags vmaFlags,
  u32* queueFamilyIndices,
  u32 queueFamilyIndiciesCount
){
  VkBufferCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bufferSize,
    .usage = bufferUsage,
    .sharingMode = VK_SHARING_MODE_CONCURRENT,
    .pQueueFamilyIndices = queueFamilyIndices,
    .queueFamilyIndexCount = queueFamilyIndiciesCount,
    .flags = 0,
  };
  VmaAllocationCreateInfo allocInfo = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = vmaFlags,
    .requiredFlags = memoryType,
  };
  VK_CHECK(vmaCreateBuffer(
    state->allocator,
    &createInfo,
    &allocInfo,
    &buffer->buffer,
    &buffer->memory,
    &buffer->memory_info 
  ), "Unable to create vertex buffer");
  return TRUE;
}

b8 create_exclusive_buffer(
  vulkan_state *state, 
  vulkan_buffer* buffer, 
  VkBufferUsageFlagBits bufferUsage, 
  VkDeviceSize bufferSize, 
  VkMemoryPropertyFlags memoryType,
  VmaAllocationCreateFlags vmaFlags
){
  VkBufferCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bufferSize,
    .usage = bufferUsage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .flags = 0,
  };
  VmaAllocationCreateInfo allocInfo = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = vmaFlags,
    .requiredFlags = memoryType,
  };
  VK_CHECK(vmaCreateBuffer(
    state->allocator,
    &createInfo,
    &allocInfo,
    &buffer->buffer,
    &buffer->memory,
    &buffer->memory_info 
  ), "Unable to create vertex buffer");
  return TRUE;
}

b8 copy_buffer(vulkan_state* state, vulkan_buffer* dstBuffer, vulkan_buffer* srcBuffer, VkDeviceSize dataSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset){
  if(dataSize+dstOffset > dstBuffer->memory_info.size){
    VERROR("Attempted to copy data larger than the capacity of the destination buffer");
    return FALSE;
  }else if(dataSize+srcOffset > srcBuffer->memory_info.size){
    VERROR("Attemped to copy data that's beyond the bounds of the source buffer");
    return FALSE;
  }
  VkCommandBuffer commandBuffer = begin_single_command(state, state->transferCommandPool);
  VkBufferCopy copyRegion = {
    .srcOffset = srcOffset,
    .dstOffset = dstOffset,
    .size = dataSize,
  };
  vkCmdCopyBuffer(commandBuffer, srcBuffer->buffer, dstBuffer->buffer, 1, &copyRegion);
  end_single_command(state, state->transferCommandPool, state->transferQueue, commandBuffer);
  return TRUE;
}

b8 create_vertex_index_buffer(vulkan_state *state){
  const VkDeviceSize bufferSize = VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE;
  if(state->transferQueueIndex == state->graphicsQueueIndex){
    VEL_CHECK(create_exclusive_buffer(
      state,
      &state->vertexIndexBuffer,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      bufferSize,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      0
    ));
  }else{
    u32 queueFamilyIndices[] = {
      state->graphicsQueueIndex,
      state->transferQueueIndex
    };
    VEL_CHECK(create_shared_buffer(
      state,
      &state->vertexIndexBuffer,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      bufferSize,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      0,
      queueFamilyIndices,
      2
    ));
  }
  /*vulkan_buffer vertexStagingBuffer = {0};
  VEL_CHECK(create_exclusive_buffer(
    state,
    &vertexStagingBuffer,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    vertexBufferSize,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  ));

  VK_CHECK(vmaCopyMemoryToAllocation(
    state->allocator,
    vertices,
    vertexStagingBuffer.memory,
    0,
    vertexBufferSize
  ), "Unable to fill vertex staging buffer with vertex data");
  vulkan_buffer indexStagingBuffer = {0};
  VEL_CHECK(create_exclusive_buffer(
    state,
    &indexStagingBuffer,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    indexBufferSize,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  ));

  VK_CHECK(vmaCopyMemoryToAllocation(
    state->allocator,
    indices,
    indexStagingBuffer.memory,
    0,
    indexBufferSize
  ), "Unable to fill index staging buffer with index data");

  VEL_CHECK(copy_buffer(
    state, 
    &state->vertexIndexBuffer, 
    &vertexStagingBuffer,
    vertexBufferSize,
    0,
    0
  ));
  VEL_CHECK(copy_buffer(
    state, 
    &state->vertexIndexBuffer, 
    &indexStagingBuffer,
    indexBufferSize,
    0,
    vertexBufferSize
  ));
  destroy_vulkan_buffer(state, &vertexStagingBuffer);
  destroy_vulkan_buffer(state, &indexStagingBuffer);*/
  return TRUE;
}

b8 register_mesh(render_state* state, vmesh mesh, u64 *outHandle){
  vulkan_state *vk_state = (vulkan_state*)state->internal_render_state;
  VkDeviceSize vertexSize = mesh.vertexCount*sizeof(vertex);
  VkDeviceSize indexSize = mesh.indexCount*sizeof(u32);
  vulkan_buffer vertexStagingBuffer = {0};
  VEL_CHECK(create_exclusive_buffer(
    vk_state,
    &vertexStagingBuffer,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    vertexSize,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  ));

  VK_CHECK(vmaCopyMemoryToAllocation(
    vk_state->allocator,
    mesh.vertices,
    vertexStagingBuffer.memory,
    0,
    vertexSize
  ), "Unable to fill vertex staging buffer with vertex data");
  vulkan_buffer indexStagingBuffer = {0};
  VEL_CHECK(create_exclusive_buffer(
    vk_state,
    &indexStagingBuffer,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    indexSize,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  ));

  VK_CHECK(vmaCopyMemoryToAllocation(
    vk_state->allocator,
    mesh.indicies,
    indexStagingBuffer.memory,
    0,
    indexSize
  ), "Unable to fill index staging buffer with index data");

  VEL_CHECK(copy_buffer(
    vk_state, 
    &vk_state->vertexIndexBuffer, 
    &vertexStagingBuffer,
    vertexSize,
    0,
    vk_state->vertexBufferUsedSize
  ));
  VEL_CHECK(copy_buffer(
    vk_state, 
    &vk_state->vertexIndexBuffer, 
    &indexStagingBuffer,
    indexSize,
    0,
    vk_state->indexBufferUsedSize
  ));
  vk_state->meshes[vk_state->curMeshIndex].indexCount = mesh.indexCount;
  vk_state->meshes[vk_state->curMeshIndex].indexOffset = vk_state->indexBufferUsedSize;
  vk_state->meshes[vk_state->curMeshIndex].vertexOffset = vk_state->vertexBufferUsedSize;
  (*outHandle) = vk_state->curMeshIndex;
  vk_state->curMeshIndex++;
  vk_state->vertexBufferUsedSize += vertexSize;
  vk_state->indexBufferUsedSize += indexSize;
  destroy_vulkan_buffer(vk_state, &vertexStagingBuffer);
  destroy_vulkan_buffer(vk_state, &indexStagingBuffer);
  return TRUE;
}

b8 create_descriptor_set_layout(vulkan_state *state){
  u32 descriptorCount = 2;
  VkDescriptorSetLayoutBinding layoutBindings[descriptorCount];
  layoutBindings[0] = (VkDescriptorSetLayoutBinding){
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = VELORA_MAX_OBJECTS,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers = VK_NULL_HANDLE,
  };
  
  layoutBindings[1] = (VkDescriptorSetLayoutBinding){
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = VELORA_MAX_TEXTURES,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers = VK_NULL_HANDLE,
  };
  VkDescriptorSetLayoutCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = descriptorCount,
    .pBindings = layoutBindings,
  };
  VK_CHECK(
    vkCreateDescriptorSetLayout(
      state->logicalDevice,
      &createInfo,
      NULL,
      &state->descriptorSetLayout
    ), "Unable to create descriptor set layout"
  );
  return TRUE;
}

b8 create_uniform_buffers(vulkan_state *state){
  VkDeviceSize minUBOAlignment = state->physicalDeviceProps.limits.minUniformBufferOffsetAlignment;
  VkDeviceSize uboAlignedSize = sizeof(ubo)+(minUBOAlignment-(sizeof(ubo)%minUBOAlignment));
  VEL_CHECK(create_exclusive_buffer(
    state,
    &state->uniformBuffer,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    uboAlignedSize*VELORA_MAX_OBJECTS*MAX_FRAMES_IN_FLIGHT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
  ));
  return TRUE;
}

b8 copy_buffer_to_image(vulkan_state* state, vulkan_buffer* buffer, vulkan_image* image, u32 width, u32 height){
  VkCommandBuffer commandBuffer = begin_single_command(state, state->transferCommandPool);
  
  VkBufferImageCopy region = {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,

    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount = 1,

    .imageOffset = {0,0,0},
    .imageExtent = {
      width,
      height,
      1
    },
  };

  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer->buffer,
    image->image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );
  
  end_single_command(
    state,
    state->transferCommandPool,
    state->transferQueue,
    commandBuffer
  );
  return TRUE;
}

b8 create_sampler(vulkan_state* state, vulkan_image* image){
  VkPhysicalDeviceProperties props = {0};
  vkGetPhysicalDeviceProperties(state->physicalDevice, &props);

  VkSamplerCreateInfo samplerInfo = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = props.limits.maxSamplerAnisotropy,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias = 0.0f,
    .minLod = 0.0f,
    .maxLod = 0.0f,
  };
  VK_CHECK(vkCreateSampler(
    state->logicalDevice, 
    &samplerInfo, 
    VK_NULL_HANDLE,
    &image->sampler
  ), "Unable to create image sampler");
  return TRUE;
}

b8 create_texture(vulkan_state* state, const char* filePath, vulkan_image* image){
  velora_pixels imageData = {0};
  import_pixels(filePath, &imageData);
  vulkan_buffer stagingBuffer;
  create_exclusive_buffer(
    state,
    &stagingBuffer,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    imageData.size,
    0,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  );
  vmaCopyMemoryToAllocation(
    state->allocator,
    imageData.pixels,
    stagingBuffer.memory,
    0,
    imageData.size
  );

  if(state->graphicsQueueIndex == state->transferQueueIndex){
    VEL_CHECK(create_exclusive_image(
      state,
      image,
      imageData.width,
      imageData.height,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    ));
  }else{
    u32 queueFamilies[2] = {
      state->graphicsQueueIndex,
      state->transferQueueIndex
    };
    VEL_CHECK(create_shared_image(
      state,
      image,
      imageData.width,
      imageData.height,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      queueFamilies,
      2
    ));
  }
  VEL_CHECK(transition_image_layout(
    state,
    image,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  ));
  VEL_CHECK(copy_buffer_to_image(
    state,
    &stagingBuffer,
    image,
    imageData.width,
    imageData.height
  ));
  VEL_CHECK(transition_image_layout(
    state, 
    image, 
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  ));

  destroy_vulkan_buffer(state, &stagingBuffer);
  free_pixels(&imageData);
  
  VEL_CHECK(create_image_view(state, image->image, VK_FORMAT_R8G8B8A8_SRGB, &image->view, VK_IMAGE_ASPECT_COLOR_BIT));

  VEL_CHECK(create_sampler(state, image));

  return TRUE;
}

b8 create_descriptor_pool(vulkan_state* state){
  u32 poolCount = 2;
  VkDescriptorPoolSize poolSize[poolCount];
  
  poolSize[0] = (VkDescriptorPoolSize){
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = MAX_FRAMES_IN_FLIGHT*VELORA_MAX_OBJECTS,
  };
  poolSize[1] = (VkDescriptorPoolSize){
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = MAX_FRAMES_IN_FLIGHT*VELORA_MAX_TEXTURES,
  };

  VkDescriptorPoolCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = poolCount,
    .pPoolSizes = poolSize,
    .maxSets = MAX_FRAMES_IN_FLIGHT,
    .flags = 0,
  };
  VK_CHECK(vkCreateDescriptorPool(
    state->logicalDevice,
    &createInfo,
    NULL,
    &state->descriptorPool
  ), "Unable to create descriptor pool");
  return TRUE;
}

b8 create_descriptor_sets(vulkan_state* state){
  state->descriptorSets = vallocate(sizeof(VkDescriptorSet)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    vcopy_memory(&layouts[i], &state->descriptorSetLayout, sizeof(VkDescriptorSetLayout));
  }
  VkDescriptorSetAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = state->descriptorPool,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts = layouts,
  };
  VK_CHECK(vkAllocateDescriptorSets(
    state->logicalDevice,
    &allocInfo,
    state->descriptorSets
  ), "Unable to allocate the descriptor sets");
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    VkDeviceSize minUBOAlignment = state->physicalDeviceProps.limits.minUniformBufferOffsetAlignment;
    VkDeviceSize uboAlignedSize = sizeof(ubo)+(minUBOAlignment-(sizeof(ubo)%minUBOAlignment));
    VkDeviceSize singleUBOBufferSize = uboAlignedSize*VELORA_MAX_OBJECTS;
    VkDescriptorBufferInfo bufferInfo[VELORA_MAX_OBJECTS];
    for(int j = 0; j < VELORA_MAX_OBJECTS; j++){
      bufferInfo[j].buffer = state->uniformBuffer.buffer;
      bufferInfo[j].offset = (singleUBOBufferSize*i)+(uboAlignedSize*j);
      bufferInfo[j].range = sizeof(ubo);
    }
    create_texture(state, "RenderRes/DefaultTexture.jpg", &state->defaultTexture);
    VkDescriptorImageInfo imageInfo[VELORA_MAX_TEXTURES] = {0};
    for(int j = 0; j < VELORA_MAX_TEXTURES; j++){
      imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo[j].imageView = state->defaultTexture.view;
      imageInfo[j].sampler = state->defaultTexture.sampler;
    }
    u32 descCount = 2;
    VkWriteDescriptorSet descriptorWrite[2];
    
    descriptorWrite[0] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = state->descriptorSets[i],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = VELORA_MAX_OBJECTS,
      .pBufferInfo = bufferInfo,
      .pImageInfo = VK_NULL_HANDLE,
      .pTexelBufferView = VK_NULL_HANDLE,
    };
    descriptorWrite[1] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = state->descriptorSets[i],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = VELORA_MAX_TEXTURES,
      .pBufferInfo = VK_NULL_HANDLE,
      .pImageInfo = imageInfo,
      .pTexelBufferView = VK_NULL_HANDLE,
    };
    vkUpdateDescriptorSets(
      state->logicalDevice, 
      descCount, 
      descriptorWrite, 
      0, 
      VK_NULL_HANDLE
    );
  }
  return TRUE;
}



typedef struct _window_dimensions {
  u32 width;
  u32 height;
} window_dimensions;

#ifdef VPLATFORM_WINDOWS
b8 create_window_surface(vulkan_state* state, platform_state* plat_internal_state, window_dimensions *outDim){
  VkWin32SurfaceCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hwnd = plat_internal_state->window,
    .hinstance = plat_internal_state->instance,
  };
  VK_CHECK(
    vkCreateWin32SurfaceKHR(state->instance, &createInfo, NULL, &state->surface), 
    "Unable to create windows surface for vulkan renderer"
  );
  RECT winSize;
  GetClientRect(plat_internal_state->window, &winSize);
  outDim->width = winSize.right;
  outDim->height = winSize.bottom;
  return TRUE;
}
#elif VPLATFORM_LINUX
b8 create_window_surface(vulkan_state* state, platform_state* plat_internal_state, window_dimensions *outDim){
  VkXlibSurfaceCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .flags = 0,
    .dpy = plat_internal_state->dis,
    .window = plat_internal_state->win,
  };
  VK_CHECK(
    vkCreateXlibSurfaceKHR(state->instance, &createInfo, NULL, &state->surface), 
    "Unable to create XLib surface for vulkan renderer"
  );
  XWindowAttributes winAtt;
  XGetWindowAttributes(plat_internal_state->dis, plat_internal_state->win, &winAtt);
  outDim->width = winAtt.width;
  outDim->height = winAtt.height;
  return TRUE;
}
#endif //VPLATFORM_*

b8 initiate_render_system(render_state* state, const char* application_name, platform_state* plat_internal_state){
  state->internal_render_state = vallocate(sizeof(vulkan_state), MEMORY_TAG_RENDERER);
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;

  vk_state->curMeshIndex = 0;
  vk_state->vertexBufferUsedSize = 0;
  vk_state->indexBufferUsedSize = 0;
  vk_state->currentFrame = 0;
  //TODO: This is horrendous, delete it entirely when we load objects from HDD
  /*vertex vertices[] = {
    { {{-0.5f, -0.5f, 0.0f}}, {{1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f}} },
    { {{0.5f, -0.5f, 0.0f}},  {{0.0f, 1.0f, 0.0f}}, {{0.0f, 0.0f}} },
    { {{0.5f, 0.5f, 0.0f}},   {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f}} },
    { {{-0.5f, 0.5f, 0.0f}},  {{1.0f, 1.0f, 1.0f}}, {{1.0f, 1.0f}} },

    { {{-0.5f, -0.5f, -0.5f}}, {{1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f}} },
    { {{0.5f, -0.5f, -0.5f}},  {{0.0f, 1.0f, 0.0f}}, {{0.0f, 0.0f}} },
    { {{0.5f, 0.5f, -0.5f}},   {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f}} },
    { {{-0.5f, 0.5f, -0.5f}},  {{1.0f, 1.0f, 1.0f}}, {{1.0f, 1.0f}} },
  };
  u32 indices[] = {
    2,1,0,0,3,2,
    6,5,4,4,7,6
  };
  vk_state->vertexCount = 8;
  vk_state->indexCount = 12;*/

  const char* deviceExtensions[10];
  u32 extensionCount = 0;

  enable_optional_feature(deviceExtensions, &extensionCount, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VEL_CHECK(create_vulkan_instance(vk_state, application_name));
  window_dimensions winAtt;
  VEL_CHECK(create_window_surface(vk_state, plat_internal_state, &winAtt));
  VEL_CHECK(obtain_physical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_logical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_vma_allocator(vk_state));
  VEL_CHECK(create_swapchain(vk_state, winAtt.width, winAtt.height));
  VEL_CHECK(create_swapchain_image_views(vk_state));
  VEL_CHECK(create_render_pass(vk_state));
  VEL_CHECK(create_descriptor_set_layout(vk_state));
  VEL_CHECK(create_graphics_pipeline(vk_state));
  VEL_CHECK(create_command_pool(vk_state));
  VEL_CHECK(create_command_buffer(vk_state));
  VEL_CHECK(create_depth_resources(vk_state));
  VEL_CHECK(create_frame_buffers(vk_state));
  VEL_CHECK(create_sync_objects(vk_state));
  VEL_CHECK(create_vertex_index_buffer(vk_state));
  //VEL_CHECK(create_texture(vk_state, "texture.jpg", &vk_state->texImage));
  VEL_CHECK(create_uniform_buffers(vk_state));
  VEL_CHECK(create_descriptor_pool(vk_state));
  VEL_CHECK(create_descriptor_sets(vk_state));
  return TRUE;
}

void shutdown_render_system(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  vkDeviceWaitIdle(vk_state->logicalDevice);
  destroy_vulkan_image(vk_state, &vk_state->depthImage);
  //destroy_vulkan_image(vk_state, &vk_state->texImage);
  vfree(vk_state->descriptorSets, sizeof(VkDescriptorSet)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vkDestroyDescriptorPool(vk_state->logicalDevice, vk_state->descriptorPool, NULL);
  vkDestroyDescriptorSetLayout(vk_state->logicalDevice, vk_state->descriptorSetLayout, NULL);
  destroy_vulkan_buffer(vk_state, &vk_state->vertexIndexBuffer);
  destroy_vulkan_buffer(vk_state, &vk_state->uniformBuffer);
  destroy_sync_objects(vk_state);
  vkDestroyCommandPool(vk_state->logicalDevice, vk_state->graphicsCommandPool, NULL);
  vkDestroyCommandPool(vk_state->logicalDevice, vk_state->transferCommandPool, NULL);
  vfree(vk_state->commandBuffer, sizeof(VkCommandBuffer)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vkDestroyPipeline(vk_state->logicalDevice, vk_state->graphicsPipeline, NULL);
  vkDestroyPipelineLayout(vk_state->logicalDevice, vk_state->pipelineLayout, NULL);
  vkDestroyRenderPass(vk_state->logicalDevice, vk_state->renderPass, NULL);
  destroy_swapchain(vk_state);
  vfree(
    vk_state->swapchainSupportDetails.surfaceFormats, 
    sizeof(VkSurfaceFormatKHR)*vk_state->swapchainSupportDetails.surfaceFormatCount, 
    MEMORY_TAG_RENDERER
  );
  vfree(
    vk_state->swapchainSupportDetails.presentModes, 
    sizeof(VkPresentModeKHR)*vk_state->swapchainSupportDetails.presentModeCount, 
    MEMORY_TAG_RENDERER
  );
  vkDestroySurfaceKHR(vk_state->instance, vk_state->surface, NULL);
  vmaDestroyAllocator(vk_state->allocator);
  vkDestroyDevice(vk_state->logicalDevice, NULL);
  #ifdef _DEBUG
  DestroyDebugUtilsMessengerEXT(vk_state->instance, vk_state->debugMessenger, NULL);
  #endif
  vkDestroyInstance(vk_state->instance, NULL);
  vfree(state->internal_render_state, sizeof(vulkan_state), MEMORY_TAG_RENDERER);
}

/*void update_uniform_buffer(vulkan_state* state){
  vec3 position = {{0,0,-5}};
  vec3 rotation = {{0,30,0}};
  quat rotationQuat = euler_to_quat(rotation);
  vec3 scale = {{1,1,1}};
  mat4x4 modelMatrix = model_matrix(position, rotationQuat, scale);
  vec3 cameraPosition = {{0,0,0}};
  vec3 cameraRotation = {{0,0,0}};
  vec3 cameraScale = {{1,1,1}};
  mat4x4 cameraModelMatrix = model_matrix(cameraPosition, euler_to_quat(cameraRotation), cameraScale);
  mat4x4 viewMatrix = {0};
  matrix4_invert(cameraModelMatrix, &viewMatrix);
  f32 aspectRatio = (float)state->swapchainExtent.width/(float)state->swapchainExtent.height;
  mat4x4 projMatrix = perspective_projection_matrix(0.1, 30, 30, aspectRatio);
  //orthographic_projection_matrix(0.1, 30, 2, aspectRatio);//perspective_projection_matrix(0.1, 30, 30, aspectRatio);
  ubo* uniformBufferMemory = (ubo*)state->uniformBuffer.memory_info.pMappedData;
  uniformBufferMemory[state->currentFrame].model = modelMatrix;
  uniformBufferMemory[state->currentFrame].view = viewMatrix;
  uniformBufferMemory[state->currentFrame].proj = projMatrix;
}*/

b8 start_recording_command_buffer(vulkan_state* state, u32 swapchainImageIndex, VkCommandBuffer *outBuffer){
  VkCommandBuffer buffer = state->commandBuffer[state->currentFrame];
  vkResetCommandBuffer(buffer, 0);
  if(swapchainImageIndex >= state->swapchainImageCount){
    VFATAL("Attempted to record command buffer for swapchain image that doesn't exist");
    return FALSE;
  }
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = NULL,
  };
  VK_CHECK(vkBeginCommandBuffer(buffer, &beginInfo), "Unable to start recording commander buffer");

  VkClearValue clearColor[2];
  clearColor[0].color =  (VkClearColorValue){{0.0f,0.0f,0.0f,1.0f}};
  clearColor[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
  VkRenderPassBeginInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = state->renderPass,
    .framebuffer = state->frameBuffers[swapchainImageIndex],
    .renderArea.offset = {0,0},
    .renderArea.extent = state->swapchainExtent,
    .clearValueCount = 2,
    .pClearValues = clearColor,
  };
  vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)state->swapchainExtent.width,
    .height = (float)state->swapchainExtent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(buffer, 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0,0},
    .extent = state->swapchainExtent,
  };
  vkCmdSetScissor(buffer, 0, 1, &scissor);
  (*outBuffer) = buffer;
  return TRUE;
}

b8 stop_recording_command_buffer(VkCommandBuffer buffer){
  vkCmdEndRenderPass(buffer);
  VK_CHECK(vkEndCommandBuffer(buffer), "Unable to end recording of command buffer");
  return TRUE;
}

b8 render_preframe(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  if(vk_state->windowResized){
    if(vk_state->newHeight == 0 && vk_state->newWidth == 0){
      vk_state->windowMinimized = TRUE;
      return TRUE;
    }else{
      vk_state->windowMinimized = FALSE;
    }
    VEL_CHECK(recreate_swapchain(vk_state, vk_state->newWidth, vk_state->newHeight));
    vk_state->windowResized = FALSE;
  }
  vkWaitForFences(vk_state->logicalDevice, 1, &vk_state->inFlight[vk_state->currentFrame], VK_TRUE, U64_MAX);
  vkResetFences(vk_state->logicalDevice, 1, &vk_state->inFlight[vk_state->currentFrame]);
  return TRUE;
}

b8 render_frame(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  vcamera *activeCamera = camera_get_active();
  mat4x4 cameraModelMatrix = transform_get_model_matrix(activeCamera->transform);
  if(vk_state->windowMinimized){
    return TRUE;
  }
  u32 imageIndex = 0;
  VkResult imageErr = vkAcquireNextImageKHR(
    vk_state->logicalDevice,
    vk_state->swapchain,
    U64_MAX,
    vk_state->imageAvailable[vk_state->currentFrame], //signal this semaphore once we get the image index
    VK_NULL_HANDLE, // A fence to signal, we don't use it
    &imageIndex
  );
  if(imageErr != VK_SUCCESS && imageErr != VK_ERROR_OUT_OF_DATE_KHR && imageErr != VK_SUBOPTIMAL_KHR){
    VFATAL("Unable to get next image from the swapchain");
    VFATAL("Vulkan Error: %s", string_VkResult(imageErr));
    return FALSE;
  }
  VkCommandBuffer buffer;
  if(start_recording_command_buffer(vk_state, imageIndex, &buffer) == FALSE){
    return FALSE;
  }

  mat4x4 viewMatrix = {0};
  matrix4_invert(cameraModelMatrix, &viewMatrix);
  f32 aspectRatio = (float)vk_state->swapchainExtent.width/(float)vk_state->swapchainExtent.height;
  mat4x4 projMatrix = perspective_projection_matrix(0.1, 30, 30, aspectRatio);
  mat4x4 viewProjMatrix = matrix4_multiply(viewMatrix, projMatrix);
  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state->graphicsPipeline);
  VkBuffer vertexBuffers[] = {vk_state->vertexIndexBuffer.buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(buffer, vk_state->vertexIndexBuffer.buffer, VERTEX_BUFFER_SIZE, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(
    buffer, 
    VK_PIPELINE_BIND_POINT_GRAPHICS, 
    vk_state->pipelineLayout,
    0,
    1,
    &vk_state->descriptorSets[vk_state->currentFrame],
    0,
    VK_NULL_HANDLE
  );
  darray *renderables;
  if(get_components(VELORA_COMPONENT_RENDERABLE, &renderables) == TRUE){
    iterator renIt = darray_create_iterator(renderables);
    vrenderable *itObj;
    u64 curObjIndex = 0;
    while(iterator_next(&renIt, (void**)&itObj)){
      mat4 modelMatrix = transform_get_model_matrix(itObj->transform);
      mat4 mvpMatrix = matrix4_multiply(modelMatrix, viewProjMatrix);
      ubo* uniformBufferMemory = (ubo*)vk_state->uniformBuffer.memory_info.pMappedData;
      u64 curUBOIndex = (VELORA_MAX_OBJECTS* vk_state->currentFrame)+(curObjIndex);
      uniformBufferMemory[curUBOIndex].mvpMat = mvpMatrix;
      uniformBufferMemory[curObjIndex].textureIndex = vk_state->materials[itObj->materialHandle].baseColorTextureIndex;
      push_constant push = {
        .uboIndex = curUBOIndex,
      };
      vkCmdPushConstants(buffer, vk_state->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
      vulkan_mesh curMesh = vk_state->meshes[itObj->meshHandle];
      vkCmdDrawIndexed(buffer, curMesh.indexCount, 1, curMesh.indexOffset, curMesh.vertexOffset, 0);
    }
  }


  stop_recording_command_buffer(buffer);
  
  VkSemaphore waitSemaphores[] = {vk_state->imageAvailable[vk_state->currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {vk_state->renderFinished[imageIndex]};
  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = waitSemaphores,
    .pWaitDstStageMask = waitStages,
    .commandBufferCount = 1,
    .pCommandBuffers = &vk_state->commandBuffer[vk_state->currentFrame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signalSemaphores,
  };
  VK_CHECK(
    vkQueueSubmit(vk_state->graphicsQueue, 1, &submitInfo, vk_state->inFlight[vk_state->currentFrame]),
    "Failed to submit draw command buffer"
  );
  VkSwapchainKHR swapChains[] = {vk_state->swapchain};
  VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signalSemaphores,
    .swapchainCount = 1,
    .pSwapchains = swapChains,
    .pImageIndices = &imageIndex,
    .pResults = NULL, // This is only used when multiple swapchains are used
  };
  VkResult err = vkQueuePresentKHR(vk_state->presentQueue, &presentInfo);
  if(err != VK_SUCCESS && err != VK_ERROR_OUT_OF_DATE_KHR && err != VK_SUBOPTIMAL_KHR){
    VFATAL("Unable to present image to swapchain");
    VFATAL("Vulkan Error: %s", string_VkResult(imageErr));
    return FALSE;
  }
  return TRUE;
}

b8 render_postframe(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  if(vk_state->windowMinimized){
    return TRUE;
  }
  vk_state->currentFrame = (vk_state->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  return TRUE;
}


#endif