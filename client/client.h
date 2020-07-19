
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <string.h>
#include <stdbool.h>
#include <netdb.h> 
#include <pthread.h>
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <signal.h>
#include "client_game.h"
#include "../coms/coms_wrapper.h"
#include "../coms/commands.h"


bool authenticate();
void flush_input();
void play_game();
void reveal_tile_option(int sock_fd, Game * game);
void flag_tile_option(int sock_fd, Game * game);
int get_leaderboard();
void connect_to_server(char* ip, char* port);
void disconnect_from_server();
