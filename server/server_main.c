#include "server_connect.h"
#include <signal.h>

#define RAND_SEED 65
#define DEFAULT_PORT  "12345"


int main(int argc, char **argv){
    // seeding random variable with defined constant
    srand(RAND_SEED); 

    // declaring which function to call when CTRL-C Is pressed
    signal(SIGINT, close_server);

    // initialising server.
    if (argc == 2){
        server_socket_init(argv[1], &server_socket);
    }
    else{
        server_socket_init(DEFAULT_PORT, &server_socket);
    }

    // initialising semaphores. 
    semaphore_init();

    // create threads to handle connections.
    create_handler_threads();

    // enter main loop to accept new connections.
    listen_for_connections();
}


