#pragma once

#include "defines.h"
#include "platform/platform.h"

#define LOG_WARN_ENABLED 1  
#define LOG_INFO_ENABLED 1  
#define LOG_DEBUG_ENABLED 1  
#define LOG_TRACE_ENABLED 1  

#if VRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

b8 initialize_logging();
void shutdown_logging();

void log_output(log_level level, const char* message, ...);

#ifndef VFATAL
#define VFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#endif

#ifndef VERROR
#define VERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
#define VWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define VWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define VINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define VINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define VDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define VDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#define VTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define VTRACE(message, ...)
#endif

#define VEL_CHECK_MSG(expr, msg, ...) \
  if(expr == FALSE){                  \
    VERROR(msg, ##__VA_ARGS__);       \
    return FALSE;                     \
  } 