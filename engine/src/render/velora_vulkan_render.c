#include "defines.h"

#ifdef VULKAN_RENDER
#include "velora_render.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include <vulkan/vulkan.h>
#include "utils/vstring.h"
#include "vk_mem_alloc.h"
#include "utils/vint.h"
#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#elif VPLATFORM_LINUX
#include <vulkan/vulkan_wayland.h>
#endif

#define VELORA_VULKAN_API_VERSION VK_API_VERSION_1_4

#define VEL_CHECK(expr) \
  if(expr == FALSE){    \
    return FALSE;       \
  }                     

#define VK_CHECK(expr, msg) \
  if(expr != VK_SUCCESS){   \
    VFATAL(msg);            \
    return FALSE;           \
  }                       

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
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VmaAllocator allocator;
  VkSurfaceKHR surface;
  swapchain_support swapchainSupportDetails;
  VkFormat swapchainFormat;
  VkImage* swapchainImages;
  u32 swapchainImageCount;
  VkExtent2D swapchainExtent;
  VkSwapchainKHR swapchain;
  VkImageView* swapchainImageViews;
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
  u8 presentQueueObtained = FALSE;
  VkBool32 isPresentCapable;

  for(int i = 0; i < queueFamilyCount; i++){
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, state->surface, &isPresentCapable), "Unable to check if this device is present capable");
    if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
      state->graphicsQueueIndex = i;
      graphicsQueueObtained = TRUE;
    }
    if(isPresentCapable){
      state->presentQueueIndex = i;
      presentQueueObtained = TRUE;
    }
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

  return (swapchainSuitable && graphicsQueueObtained && presentQueueObtained && (foundExtensions == extensionCount));
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
  activate_queue(queueCreateInfos, &queueCount, state->presentQueueIndex, 1, &queuePri);
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
  VK_CHECK(vkCreateDevice(state->physicalDevice, &logicalDeviceCreateInfo, NULL, &state->logicalDevice), "Unable to create logical device");
  vkGetDeviceQueue(state->logicalDevice, state->graphicsQueueIndex, 0, &state->graphicsQueue);
  vkGetDeviceQueue(state->logicalDevice, state->presentQueueIndex, 0, &state->presentQueue);
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
  return TRUE;
}
#endif //VPLATFORM_WINDOWS

void shutdown_render_system(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  for(int i = 0; i < vk_state->swapchainImageCount; i++){
    vkDestroyImageView(vk_state->logicalDevice, vk_state->swapchainImageViews[i], NULL);
  }
  vkDestroySwapchainKHR(vk_state->logicalDevice, vk_state->swapchain, NULL);
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

#endif