#include "defines.h"

#ifdef VULKAN_RENDER
#include "velora_render.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include <vulkan/vulkan.h>
#include "utils/vstring.h"
#include "vk_mem_alloc.h"
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



u8 create_swapchain(vulkan_state* state, u32 width, u32 height){
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
  LPRECT winSize;
  GetClientRect(window, winSize);
  VEL_CHECK(create_swapchain(vk_state, winSize->right, winSize->bottom));
  return TRUE;
}
#endif //VPLATFORM_WINDOWS

void shutdown_render_system(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
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