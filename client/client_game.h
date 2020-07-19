#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../coms/coms_wrapper.h"
#include "../coms/game_types.h"
#include "../coms/commands.h"

void game_init(Game * game);
void free_game(Game *game);
int recieve_tiles(int sock_fd, Game * game);
void send_coordinates(int sock_fd);
void print_board(Game *game);
bool game_won(Game * game);
