/* Server Connect header file */ 

#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "server_game.h"
#include "leaderboard.h"
#include "../coms/commands.h"
#include "../coms/coms_wrapper.h"

#define MAX_CLIENTS 10

/*=========================*
 *Global External Variables*
 *=========================*/
extern int      server_socket;

/*==============================*
 *Structures used in the program*
 *==============================*/
struct client_queue{
    int     fd;
    struct  client_queue *next;
};

struct connection_inst{
    int     thread;         // allocated thread 
    int     fd;             // descriptor of socket associated with client.
    char    username[40];   // username of client currently being serviced.
    char    password[40];   // password of client currently being serviced.
    bool    connect;        // stores whether a client is currently connected to socket. 
};

typedef struct connection_inst connection_t; 
typedef struct client_queue client_queue_t;

/*=====================*
 *Function declarations*
 *=====================*/
void listen_for_connections( void );
void server_socket_init( char * port, int *ptr_sock );
void semaphore_init( void );
void create_handler_threads( void );
void add_to_queue( int client_fd );
int get_next_client( void );
void *handler_loop( void* data );
int get_login_info(connection_t *thread);
int authenticated(connection_t *thread);
int client_menu_select(connection_t *thread);
void play_game_inst(connection_t *thread, Game* game);
void disconnect_thread(connection_t *thread);
void close_server( int var );
void free_resources( void );
