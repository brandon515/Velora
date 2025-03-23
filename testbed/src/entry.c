#include <entry.h>
#include "game.h"

#include <core/vmemory.h>

b8 create_game(game* out_game){
    out_game->app_config.name = "Velora";
    out_game->app_config.start_pos_height = 720;
    out_game->app_config.start_pos_width = 1280;
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;

    out_game->initialize = game_init;
    out_game->on_resize = game_win_resize;
    out_game->update = game_update;
    out_game->render = game_render;

    out_game->state = vallocate(sizeof(game_state), MEMORY_TAG_GAME);

    return TRUE;
}