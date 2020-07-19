#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include "../coms/game_types.h"
#include "../coms/commands.h"
#include "../coms/coms_wrapper.h"

int game_init(Game *game);
time_t get_elapsed_time(Game *game);
int tile_is_mine(Game *game, int x, int y);
void place_mines(Game *game);
int get_coords(int sock_fd, char * buf);
int reveal_tile(Game* game, int x, int y, int sock_fd);
int reveal_all_mines(Game *game, int sock_fd);
int reveal_tile_user(Game *game, int sock_fd);
int flag_tile_user(Game *game, int sock_fd);
int send_tile(int sock_fd, Tile * tile);
int game_won(Game * game);
