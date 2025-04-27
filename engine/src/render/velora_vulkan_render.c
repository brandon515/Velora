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
#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#elif VPLATFORM_LINUX
#include <wayland-client.h>
#include "platform/xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include <vulkan/vulkan_wayland.h>
#endif

#define VELORA_VULKAN_API_VERSION VK_API_VERSION_1_4
#define MAX_FRAMES_IN_FLIGHT 2

#define VEL_CHECK(expr) \
  if(expr == FALSE){    \
    return FALSE;       \
  }                     

#define VK_CHECK(expr, msg)                           \
  {VkResult err = expr;                               \
  if(err != VK_SUCCESS){                              \
    VFATAL(msg);                                      \
    VFATAL("Vulkan Error: %s", string_VkResult(err)); \
    return FALSE;                                     \
  }}        

typedef struct _ubo{
  mat4x4 model;
  mat4x4 view;
  mat4x4 proj;
} ubo;
typedef struct _velora_buffer{
  VkBuffer buffer;
  VmaAllocation memory;
  VmaAllocationInfo memory_info;
} velora_buffer;

typedef struct _vertex {
  vec2 pos;
  vec3 color;
} vertex;

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
  VkCommandPool commandPool;
  VkCommandBuffer* commandBuffer;
  VkCommandPool transferPool;
  VkSemaphore* imageAvailable, *renderFinished;
  VkFence* inFlight;
  u32 currentFrame;
  b8 windowResized, windowMinimized;
  u32 newWidth, newHeight;
  velora_buffer vertexIndexBuffer;
  velora_buffer uniformBuffer;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet* descriptorSets;
  u64 vertexCount;
  u64 indexCount;
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

u8 initiate_validation_callback(vulkan_state* state){
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  populate_debug_create_info(&createInfo);
  VK_CHECK(
    CreateDebugUtilsMessengerEXT(state->instance, &createInfo, NULL, &(state->debugMessenger)),
    "Unable to create the validation layer callback"
  );
  return TRUE;
}
#endif

u8 check_layer_support(const char** validation_layers, u32 num_of_layers){
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

u8 create_vulkan_instance(vulkan_state* state, const char* app_name){
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
  enable_optional_feature(extensions, &extension_count, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
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

u8 obtain_swapchain_info(vulkan_state* state, VkPhysicalDevice device){
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

u8 is_physical_device_suitable(vulkan_state* state, VkPhysicalDevice device, const char** extensions, u32 extensionCount){
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

  return (swapchainSuitable && graphicsQueueObtained && transferQueueObtained && presentQueueObtained && (foundExtensions == extensionCount));
}

u8 obtain_physical_device(vulkan_state* state, const char** extensions, u32 extensionCount){
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

u8 create_logical_device(vulkan_state* state, const char** extensions, u32 extensionCount){
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
  return TRUE;
}

u8 create_vma_allocator(vulkan_state* state){
  VmaAllocatorCreateInfo createInfo = {
    .device = state->logicalDevice,
    .physicalDevice = state->physicalDevice,
    .instance = state->instance,
    .vulkanApiVersion = VELORA_VULKAN_API_VERSION,
  };
  VK_CHECK(vmaCreateAllocator(&createInfo, &state->allocator), "Unable to create VMA allocator");
  return TRUE;
}

u8 choose_surface_format(vulkan_state* state, VkSurfaceFormatKHR *out_format){
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

u8 choose_present_mode(vulkan_state* state, VkPresentModeKHR* out_mode){
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

u8 create_swapchain(vulkan_state* state, u32 width, u32 height){
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

u8 create_swapchain_image_views(vulkan_state* state){
  state->swapchainImageViews = vallocate(sizeof(VkImageView)*state->swapchainImageCount, MEMORY_TAG_RENDERER);
  for(int i = 0; i < state->swapchainImageCount; i++){
    VkComponentMapping compMapping = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    VkImageSubresourceRange subresRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    };
    VkImageViewCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = state->swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = state->swapchainFormat,
      .components = compMapping,
      .subresourceRange = subresRange,
    };
    VK_CHECK(vkCreateImageView(
      state->logicalDevice,
      &createInfo,
      NULL,
      &state->swapchainImageViews[i]
    ), "Unable to create image view");
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
  vfree(vk_state->swapchainImages, sizeof(VkImageView)*vk_state->swapchainImageCount, MEMORY_TAG_RENDERER);
  vkDestroySwapchainKHR(vk_state->logicalDevice, vk_state->swapchain, NULL);
}

VkShaderModule get_shader_module(vulkan_state* state, const char* shaderFileName){
  FILE* shaderFile = fopen(shaderFileName, "rb");
  u64 shaderFileSize = get_file_size(shaderFile);
  u8* shaderFileContents = vallocate(shaderFileSize, MEMORY_TAG_RENDERER);
  VEL_CHECK(get_file_contents(shaderFile, shaderFileContents));
  fclose(shaderFile);
  VkShaderModuleCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shaderFileSize,
    .pCode = (u32*)shaderFileContents
  };
  VkShaderModule ret_mod;
  VK_CHECK(vkCreateShaderModule(state->logicalDevice, &createInfo, NULL, &ret_mod), "Failed to create shader module");
  vfree(shaderFileContents, shaderFileSize, MEMORY_TAG_RENDERER);
  return ret_mod;
}

u8 create_render_pass(vulkan_state* state){
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
  VkAttachmentReference colorAttachmentRef = { // This is the layout(location = 0) out vec4 outColor in the fragment shader
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentRef,
  };
  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL, // This refers to the implicit subpass at the beginning of the pipeline
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  VkRenderPassCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &colorAttachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };
  VK_CHECK(vkCreateRenderPass(state->logicalDevice, &createInfo, NULL, &state->renderPass), "Unable to create render pass");
  return TRUE;
}

u8 create_graphics_pipeline(vulkan_state* state){
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
  const u8 numOfVertexStructMembers = 2;
  VkVertexInputAttributeDescription vertexStructDesc[numOfVertexStructMembers];

  vertexStructDesc[0].binding = 0;
  vertexStructDesc[0].location = 0;
  vertexStructDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
  vertexStructDesc[0].offset = offsetof(vertex, pos);

  vertexStructDesc[1].binding = 0;
  vertexStructDesc[1].location = 1;
  vertexStructDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexStructDesc[1].offset = offsetof(vertex, color);
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
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &state->descriptorSetLayout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL,
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
    .pDepthStencilState = NULL, // We disabled stencil buffers
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

u8 create_frame_buffers(vulkan_state* state){
  state->frameBuffers = vallocate(
    sizeof(VkFramebuffer)*state->swapchainImageCount, 
    MEMORY_TAG_RENDERER
  );
  for(int i = 0; i < state->swapchainImageCount; i++){
    VkImageView attachments[] = {
      state->swapchainImageViews[i],
    };
    VkFramebufferCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = state->renderPass,
      .attachmentCount = 1,
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

u8 create_command_pool(vulkan_state* state){
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
      &state->commandPool
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
      &state->transferPool
    ),
    "Unable to create transfer command pool"
  );
  return TRUE;
}

u8 create_command_buffer(vulkan_state* state){
  state->commandBuffer = vallocate(sizeof(VkCommandBuffer)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = state->commandPool,
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

u8 record_command_buffer(vulkan_state* state, u32 swapchainImageIndex){
  if(swapchainImageIndex >= state->swapchainImageCount){
    VFATAL("Attempted to record command buffer for swapchain image that doesn't exist");
    return FALSE;
  }
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = NULL,
  };
  VK_CHECK(vkBeginCommandBuffer(state->commandBuffer[state->currentFrame], &beginInfo), "Unable to start recording commander buffer");

  VkClearValue clearColor = {{{0.0f,0.0f,0.0f,1.0f}}};
  VkRenderPassBeginInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = state->renderPass,
    .framebuffer = state->frameBuffers[swapchainImageIndex],
    .renderArea.offset = {0,0},
    .renderArea.extent = state->swapchainExtent,
    .clearValueCount = 1,
    .pClearValues = &clearColor,
  };
  vkCmdBeginRenderPass(state->commandBuffer[state->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(state->commandBuffer[state->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)state->swapchainExtent.width,
    .height = (float)state->swapchainExtent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(state->commandBuffer[state->currentFrame], 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0,0},
    .extent = state->swapchainExtent,
  };
  vkCmdSetScissor(state->commandBuffer[state->currentFrame], 0, 1, &scissor);
  VkBuffer vertexBuffers[] = {state->vertexIndexBuffer.buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(state->commandBuffer[state->currentFrame], 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(state->commandBuffer[state->currentFrame], state->vertexIndexBuffer.buffer, state->vertexCount*sizeof(vertex), VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(
    state->commandBuffer[state->currentFrame], 
    VK_PIPELINE_BIND_POINT_GRAPHICS, 
    state->pipelineLayout,
    0,
    1,
    &state->descriptorSets[state->currentFrame],
    0,
    VK_NULL_HANDLE
  );
  vkCmdDrawIndexed(state->commandBuffer[state->currentFrame], state->indexCount, 1, 0, 0, 0);
  vkCmdEndRenderPass(state->commandBuffer[state->currentFrame]);
  VK_CHECK(vkEndCommandBuffer(state->commandBuffer[state->currentFrame]), "Unable to end recording of command buffer");
  return TRUE;
}

u8 create_sync_objects(vulkan_state* state){
  state->imageAvailable = vallocate(sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  state->renderFinished = vallocate(sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  state->inFlight = vallocate(sizeof(VkFence)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  VkSemaphoreCreateInfo semInfo = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceInfo = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT, // Make it so that the render loop doesn't hang on the fence for the first frame
  };
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    VK_CHECK(
      vkCreateSemaphore(state->logicalDevice, &semInfo, NULL, &state->imageAvailable[i]),
      "Unable to create Semaphore"
    );
    VK_CHECK(
      vkCreateSemaphore(state->logicalDevice, &semInfo, NULL, &state->renderFinished[i]),
      "Unable to create Semaphore"
    );
    VK_CHECK(
      vkCreateFence(state->logicalDevice, &fenceInfo, NULL, &state->inFlight[i]),
      "Unable to create fence"
    );
  }
  return TRUE;
}

b8 recreate_swapchain(vulkan_state* state, u32 width, u32 height){
  vkDeviceWaitIdle(state->logicalDevice);

  destroy_swapchain(state);
  VEL_CHECK(obtain_swapchain_info(state, state->physicalDevice));
  VEL_CHECK(create_swapchain(state, width, height));
  VEL_CHECK(create_swapchain_image_views(state));
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

void destroy_velora_buffer(vulkan_state* state, velora_buffer* buffer){
  vmaDestroyBuffer(
    state->allocator, 
    buffer->buffer,
    buffer->memory
  );
}

b8 create_shared_buffer(
  vulkan_state *state, 
  velora_buffer* buffer, 
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
  velora_buffer* buffer, 
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

b8 copy_buffer(vulkan_state* state, velora_buffer* dstBuffer, velora_buffer* srcBuffer, VkDeviceSize dataSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset){
  if(dataSize+dstOffset > dstBuffer->memory_info.size){
    VERROR("Attempted to copy data larger than the capacity of the destination buffer");
    return FALSE;
  }else if(dataSize+srcOffset > srcBuffer->memory_info.size){
    VERROR("Attemped to copy data that's beyond the bounds of the source buffer");
    return FALSE;
  }
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = state->transferPool,
    .commandBufferCount = 1,
  };
  VkCommandBuffer commandBuffer;
  VK_CHECK(
    vkAllocateCommandBuffers(state->logicalDevice, &allocInfo, &commandBuffer),
    "Unable to allocate command buffer from Transfer command pool"
  );
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  VK_CHECK(
    vkBeginCommandBuffer(commandBuffer, &beginInfo),
    "Unable to start recording on transfer command buffer"
  );
  VkBufferCopy copyRegion = {
    .srcOffset = srcOffset,
    .dstOffset = dstOffset,
    .size = dataSize,
  };
  vkCmdCopyBuffer(commandBuffer, srcBuffer->buffer, dstBuffer->buffer, 1, &copyRegion);
  VK_CHECK(
    vkEndCommandBuffer(commandBuffer),
    "Unable to end transfer command buffer recording"
  );

  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer
  };
  vkQueueSubmit(state->transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(state->transferQueue);
  vkFreeCommandBuffers(
    state->logicalDevice,
    state->transferPool,
    1,
    &commandBuffer
  );
  return TRUE;
}

b8 create_vertex_index_buffer(vulkan_state *state, vertex* vertices, u16* indices){
  const VkDeviceSize vertexBufferSize = sizeof(vertex)*state->vertexCount;
  const VkDeviceSize indexBufferSize = sizeof(u16)*state->indexCount;
  const VkDeviceSize bufferSize = vertexBufferSize+indexBufferSize;
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
  velora_buffer vertexStagingBuffer = {0};
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
  velora_buffer indexStagingBuffer = {0};
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
  destroy_velora_buffer(state, &vertexStagingBuffer);
  destroy_velora_buffer(state, &indexStagingBuffer);
  return TRUE;
}

b8 create_descriptor_set_layout(vulkan_state *state){
  VkDescriptorSetLayoutBinding uboLayoutBinding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = VK_NULL_HANDLE,
  };
  VkDescriptorSetLayoutCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &uboLayoutBinding,
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
  VEL_CHECK(create_exclusive_buffer(
    state,
    &state->uniformBuffer,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    sizeof(ubo)*MAX_FRAMES_IN_FLIGHT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
  ));
  return TRUE;
}

b8 create_descriptor_pool(vulkan_state* state){
  VkDescriptorPoolSize poolSize = {
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkDescriptorPoolCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = 1,
    .pPoolSizes = &poolSize,
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
    VkDescriptorBufferInfo bufferInfo = {
      .buffer = state->uniformBuffer.buffer,
      .offset = sizeof(ubo)*i,
      .range = sizeof(ubo),
    };
    VkWriteDescriptorSet descriptorWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = state->descriptorSets[i],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &bufferInfo,
      .pImageInfo = VK_NULL_HANDLE,
      .pTexelBufferView = VK_NULL_HANDLE,
    };
    vkUpdateDescriptorSets(state->logicalDevice, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
  }
  return TRUE;
}

#ifdef VPLATFORM_WINDOWS
u8 create_window_surface(vulkan_state* state, HWND window, HINSTANCE handle){
  VkWin32SurfaceCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hwnd = window,
    .hinstance = handle,
  };
  VK_CHECK(
    vkCreateWin32SurfaceKHR(state->instance, &createInfo, NULL, &state->surface), 
    "Unable to create windows surface for vulkan renderer"
  );
  return TRUE;
}

u8 initiate_render_system(render_state* state, const char* application_name, HWND window, HINSTANCE handle){
  state->internal_render_state = vallocate(sizeof(vulkan_state), MEMORY_TAG_RENDERER);
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  vk_state->currentFrame = 0;
  //TODO: This is horrendous, delete it entirely when we load objects from HDD
  vertex vertices[] = {
    { {{-0.5f, -0.5f}}, {{1.0f, 0.0f, 0.0f}} },
    { {{0.5f, -0.5f}},  {{0.0f, 1.0f, 0.0f}} },
    { {{0.5f, 0.5f}},   {{0.0f, 0.0f, 1.0f}} },
    { {{-0.5f, 0.5f}},  {{1.0f, 1.0f, 1.0f}} },
  };
  u16 indices[] = {2,1,0,0,3,2};
  vk_state->vertexCount = 4;
  vk_state->indexCount = 6;

  const char* deviceExtensions[10];
  u32 extensionCount = 0;

  enable_optional_feature(deviceExtensions, &extensionCount, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VEL_CHECK(create_vulkan_instance(vk_state, application_name));
  VEL_CHECK(create_window_surface(vk_state, window, handle));
  VEL_CHECK(obtain_physical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_logical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_vma_allocator(vk_state));
  RECT winSize;
  GetClientRect(window, &winSize);
  VEL_CHECK(create_swapchain(vk_state, winSize.right, winSize.bottom));
  VEL_CHECK(create_swapchain_image_views(vk_state));
  VEL_CHECK(create_render_pass(vk_state));
  VEL_CHECK(create_descriptor_set_layout(vk_state));
  VEL_CHECK(create_graphics_pipeline(vk_state));
  VEL_CHECK(create_frame_buffers(vk_state));
  VEL_CHECK(create_command_pool(vk_state));
  VEL_CHECK(create_command_buffer(vk_state));
  VEL_CHECK(create_sync_objects(vk_state));
  VEL_CHECK(create_vertex_index_buffer(vk_state, vertices, indices));
  VEL_CHECK(create_uniform_buffers(vk_state));
  VEL_CHECK(create_descriptor_pool(vk_state));
  VEL_CHECK(create_descriptor_sets(vk_state));
  return TRUE;
}
#elif VPLATFORM_LINUX
u8 create_window_surface(vulkan_state* state, struct wl_display* display, struct wl_surface* surface){
  VkWin32SurfaceCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hwnd = window,
    .hinstance = handle,
  };
  VK_CHECK(
    vkCreateWin32SurfaceKHR(state->instance, &createInfo, NULL, &state->surface), 
    "Unable to create windows surface for vulkan renderer"
  );
  return TRUE;
}

u8 initiate_render_system(render_state* state, const char* application_name, struct wl_display* display, struct wl_surface* surface){
  state->internal_render_state = vallocate(sizeof(vulkan_state), MEMORY_TAG_RENDERER);
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;

  const char* deviceExtensions[10];
  u32 extensionCount = 0;

  enable_optional_feature(deviceExtensions, &extensionCount, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VEL_CHECK(create_vulkan_instance(vk_state, application_name));
  VEL_CHECK(create_window_surface(vk_state, window, handle));
  VEL_CHECK(obtain_physical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_logical_device(vk_state, deviceExtensions, extensionCount));
  VEL_CHECK(create_vma_allocator(vk_state));
  RECT winSize;
  GetClientRect(window, &winSize);
  VEL_CHECK(create_swapchain(vk_state, winSize.right, winSize.bottom));
  VEL_CHECK(create_swapchain_image_views(vk_state));
  VEL_CHECK(create_render_pass(vk_state));
  VEL_CHECK(create_graphics_pipeline(vk_state));
  VEL_CHECK(create_frame_buffers(vk_state));
  VEL_CHECK(create_command_pool(vk_state));
  VEL_CHECK(create_command_buffer(vk_state));
  VEL_CHECK(create_sync_objects(vk_state));
  return TRUE;
}
#endif //VPLATFORM_*


void shutdown_render_system(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  vkDeviceWaitIdle(vk_state->logicalDevice);
  vfree(vk_state->descriptorSets, sizeof(VkDescriptorSet)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vkDestroyDescriptorPool(vk_state->logicalDevice, vk_state->descriptorPool, NULL);
  vkDestroyDescriptorSetLayout(vk_state->logicalDevice, vk_state->descriptorSetLayout, NULL);
  destroy_velora_buffer(vk_state, &vk_state->vertexIndexBuffer);
  destroy_velora_buffer(vk_state, &vk_state->uniformBuffer);
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    vkDestroySemaphore(vk_state->logicalDevice, vk_state->imageAvailable[i], NULL);
    vkDestroySemaphore(vk_state->logicalDevice, vk_state->renderFinished[i], NULL);
    vkDestroyFence(vk_state->logicalDevice, vk_state->inFlight[i], NULL);
  }
  vfree(vk_state->imageAvailable, sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vfree(vk_state->renderFinished, sizeof(VkSemaphore)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vfree(vk_state->inFlight, sizeof(VkFence)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vkDestroyCommandPool(vk_state->logicalDevice, vk_state->commandPool, NULL);
  vkDestroyCommandPool(vk_state->logicalDevice, vk_state->transferPool, NULL);
  vfree(vk_state->commandBuffer, sizeof(VkCommandBuffer)*MAX_FRAMES_IN_FLIGHT, MEMORY_TAG_RENDERER);
  vfree(vk_state->frameBuffers, sizeof(VkFramebuffer)*vk_state->swapchainImageCount, MEMORY_TAG_RENDERER);
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
  vfree(state, sizeof(render_state), MEMORY_TAG_RENDERER);
}

#include <math.h>
void update_uniform_buffer(vulkan_state* state){
  vec3 position = {{0,0,-20}};
  vec3 rotation = {{0,0,0}};
  vec3 scale = {{1,1,1}};
  mat4x4 modelMatrix = model_matrix(position, euler_to_quat(rotation), scale);
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
}

u8 render_preframe(render_state* state){
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

u8 render_frame(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
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
  vkResetCommandBuffer(vk_state->commandBuffer[vk_state->currentFrame], 0);
  record_command_buffer(vk_state, imageIndex);
  
  VkSemaphore waitSemaphores[] = {vk_state->imageAvailable[vk_state->currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {vk_state->renderFinished[vk_state->currentFrame]};
  update_uniform_buffer(vk_state);
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

u8 render_postframe(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  if(vk_state->windowMinimized){
    return TRUE;
  }
  vk_state->currentFrame = (vk_state->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  return TRUE;
}


#endif