#include "defines.h"

#ifdef VULKAN_RENDER
#include "velora_render.h"
#include "core/logger.h"
#include <vulkan/vulkan.h>
#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#elif VPLATFORM_LINUX
#include <vulkan/vulkan_wayland.h>
#endif

static VkInstance instance;

u8 initiate_render_system(const char* application_name){
  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = application_name,
    .applicationVersion = VK_MAKE_API_VERSION(0,0,1,0),
    .pEngineName = "Velora",
    .engineVersion = VK_MAKE_API_VERSION(0,0,1,0),
    .apiVersion = VK_API_VERSION_1_4,
  };
  const char** extensions;
  u32 extension_count;
  #ifdef VPLATFORM_WINDOWS
  const char* extensionsForWin32[] = {
    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME
  };
  extensions = extensionsForWin32;
  extension_count = 2;
  #elif VPLATFORM_LINUX
  static const char *const extensionsForWayland[] = {
    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
  };
  extensions = &extensionsForWayland;
  extension_count = 2;
  #else
  VFATAL("Vulkan render backend chosen on unsupported system");
  return FALSE;
  #endif
  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = extension_count,
    .ppEnabledExtensionNames = extensions, // Extensions enabled by platform, using some pre-processor directives
    .enabledLayerCount = 0, // We'll enable this later for debug layers
  };
  if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS){
    VFATAL("Unable to start Vulkan instance");
    return FALSE;
  }
  return TRUE;
}

void shutdown_render_system(){
  vkDestroyInstance(instance, NULL);
}

#endif