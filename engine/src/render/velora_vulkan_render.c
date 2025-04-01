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

#define VEL_CHECK(expr)  \
  if(expr == FALSE){    \
    return FALSE;       \
  }                     \

typedef struct _vulkan_state{
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  u32 graphicsQueueIndex;
  VkQueue graphicsQueue;
  VmaAllocator allocator;
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
  if(CreateDebugUtilsMessengerEXT(state->instance, &createInfo, NULL, &(state->debugMessenger)) != VK_SUCCESS){
    VFATAL("Unable to create the validation layer callback");
    return FALSE;
  }
  return TRUE;
}
#endif

u8 check_layer_support(const char** validation_layers, u32 num_of_layers){
  u32 layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties availableLayers[layerCount];
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
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

u8 find_queue_family_index(VkPhysicalDevice device, VkQueueFlagBits queue_bit, u32* out_index){
  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  if(queueFamilyCount == 0){
    VFATAL("Chosen physical device has no queue families");
    return FALSE;
  }
  VkQueueFamilyProperties props[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, props);

  for(int i = 0; i < queueFamilyCount; i++){
    if(props[i].queueFlags & queue_bit){
      (*out_index) = i;
      return TRUE;
    }
  }
  return FALSE;
}

u8 is_physical_device_suitable(vulkan_state* state, VkPhysicalDevice device){
  if(find_queue_family_index(device, VK_QUEUE_GRAPHICS_BIT, &state->graphicsQueueIndex) == FALSE){
    return FALSE;
  }
  return TRUE;
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
  if(vkCreateInstance(&createInfo, NULL, &state->instance) != VK_SUCCESS){
    VFATAL("Unable to start Vulkan instance");
    return FALSE;
  }
  #ifdef _DEBUG
  if(initiate_validation_callback(state) == FALSE){
    return FALSE;
  }
  #endif
  return TRUE;
}

u8 obtain_physical_device(vulkan_state* state){
  u32 deviceCount = 0;
  vkEnumeratePhysicalDevices(state->instance, &deviceCount, NULL);
  if(deviceCount == 0){
    VFATAL("No GPU capable of supporting a vulkan renderer was found");
    return FALSE;
  }
  VkPhysicalDevice devices[deviceCount];
  vkEnumeratePhysicalDevices(state->instance, &deviceCount, devices);
  for(int i = 0; i < deviceCount; deviceCount++){
    if(is_physical_device_suitable(state, devices[i])){
      state->physicalDevice = devices[i];
      return TRUE;
    }
  }
  return FALSE;
}

u8 create_logical_device(vulkan_state* state){
  float queuePri = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = state->graphicsQueueIndex, //graphics queue
    .queueCount = 1, //only one of them please
    .pQueuePriorities = &queuePri,
  };
  VkPhysicalDeviceFeatures deviceFeatues = {0};
  VkDeviceCreateInfo logicalDeviceCreateInfo ={
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = &queueCreateInfo,
    .queueCreateInfoCount = 1,
    .pEnabledFeatures = &deviceFeatues,
    .enabledLayerCount = 0, //We would need to specify the validation layers in this if it was an older implementation of Vulkan, I can't care though
  };
  if(vkCreateDevice(state->physicalDevice, &logicalDeviceCreateInfo, NULL, &state->logicalDevice) != VK_SUCCESS){
    VFATAL("Unable to create logical device");
    return FALSE;
  }
  vkGetDeviceQueue(state->logicalDevice, state->graphicsQueueIndex, 0, &state->graphicsQueue);
  return TRUE;
}

u8 create_vma_allocator(vulkan_state* state){
  VmaAllocatorCreateInfo createInfo = {
    .device = state->logicalDevice,
    .physicalDevice = state->physicalDevice,
    .instance = state->instance,
    .vulkanApiVersion = VELORA_VULKAN_API_VERSION,
  };
  if(vmaCreateAllocator(&createInfo, &state->allocator) != VK_SUCCESS){
    return FALSE;
  }
  return TRUE;
}

u8 initiate_render_system(render_state* state, const char* application_name){
  state->internal_render_state = vallocate(sizeof(vulkan_state), MEMORY_TAG_RENDERER);
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
  VEL_CHECK(create_vulkan_instance(vk_state, application_name));
  VEL_CHECK(obtain_physical_device(vk_state));
  VEL_CHECK(create_logical_device(vk_state));
  VEL_CHECK(create_vma_allocator(vk_state));
  return TRUE;
}

void shutdown_render_system(render_state* state){
  vulkan_state* vk_state = (vulkan_state*)state->internal_render_state;
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