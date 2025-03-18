#pragma once

#include "defines.h"

typedef struct _application_config{
    i16 start_pos_x;
    i16 start_pos_y;
    i16 start_pos_width;
    i16 start_pos_height;
    const char* name;
} application_config;

VAPI b8 application_start(application_config* config);

VAPI b8 application_run();  