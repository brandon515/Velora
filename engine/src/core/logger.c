#include "logger.h"
#include "asserts.h"

//TODO: remove this import, we'll be writing a much more sophisticated logging system soon
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

b8 initialize_logging(){
  // TODO: create log file
  return TRUE;
}

void shutdown_logging(){
  // TODO: Cleanup any detritus from logging system
}

void log_output(log_level level, const char* message, ...){
  const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};

  //32k char limit should be good enough for any error
  char fmt_message[32000];
  memset(fmt_message, 0, sizeof(fmt_message));

  //There's apparently some error with clang for this so we can't just use va_list. It's fine.
  __builtin_va_list arg_ptr;

  va_start(arg_ptr, message);
  vsnprintf(fmt_message, sizeof(fmt_message), message, arg_ptr);
  va_end(arg_ptr);

  char out_message[32000];
  sprintf(out_message, "%s%s\n", level_strings[level], fmt_message);

  printf("%s", out_message);
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line){
  log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, Message: %s, in file: %s, line: %d\n", expression, message, file, line);
}