/*=============================*
 *CAB403 Alexander Brimblecombe*
 *=============================*/
#include "server_connect.h"

/*================*
 *Global Variables*
 *================*/

int server_socket; 
sem_t full;               // semaphore for waiting clients.
sem_t vacant;             // semaphore for vacant threads ready for connections.
pthread_mutex_t queue_mutex;        // provide exlusive access to client queue
client_queue_t* client_head = NULL; // head of client queue linked list
client_queue_t* client_tail = NULL; // tail of client queue linked list
pthread_t c_threads[MAX_CLIENTS];     // thread array
connection_t thread_info[MAX_CLIENTS];
bool server_running;     // whether server is running or not


/* Function that detects when a new client connects to the server and adds them to the queue */ 
/* Producing function - adds new clients to the queue */
void listen_for_connections( void ){
    socklen_t sin_size;
    struct sockaddr_in their_addr;
    int client_fd;

    while(server_running){
        // wait for a thread to become avaliable 
        sem_wait(&vacant);
        if ((client_fd = accept(server_socket, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("**Server: new connection from %s**\n", inet_ntoa(their_addr.sin_addr));
        add_to_queue(client_fd);
        sem_post(&full);
    }
}

/* Setting up the server socket to listen on the provided port */ 
void server_socket_init(char * port, int * ptr_sock){
    printf("Initialising server socket\n");
    struct  sockaddr_in my_addr;

    if ((*ptr_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");           //failed to created socket 
		exit(EXIT_FAILURE);
	}

    my_addr.sin_family = AF_INET; 
    my_addr.sin_port = htons(atoi(port));
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");             // failed to bind socket
        close(server_socket);
		exit(EXIT_FAILURE);
	}
    listen(*ptr_sock, MAX_CLIENTS);
    printf("socket listening...\n");
    server_running = true; 
}

/* Initialising the semaphores */
void semaphore_init( void ){
    sem_init(&full, 0, 0);
    sem_init(&vacant, 0, MAX_CLIENTS);
}

/* Creating threads to handle clients */ 
void create_handler_threads( void ){
    int i;
    printf("creating threads to handle connections...\n");
    for(i = 0; i < MAX_CLIENTS; i++){
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&c_threads[i], NULL, handler_loop, (void*)arg);
    }
}

/* Adding a new client to the queue */ 
void add_to_queue( int client_fd ){
    client_queue_t * new_client = malloc(sizeof(client_queue_t));

    if (!new_client){
        printf("error allocating memory");
        exit(EXIT_FAILURE);
    }
    new_client->fd = client_fd;
    new_client->next = NULL;  

    pthread_mutex_lock(&queue_mutex);    

    if (client_head == NULL){         
        client_head = new_client;
        client_tail = new_client;
    }

    else if (client_head == client_tail){  
        client_head->next = new_client; 
        client_tail = new_client; 
    }

    else{
        client_tail->next = new_client; 
        client_tail = new_client;
    }
    pthread_mutex_unlock(&queue_mutex);    //unlock queue mutex
}

/* Get descriptor for and remove next item in the queue */ 
int get_next_client( void ){
    client_queue_t *client; 
    int fd; 
    pthread_mutex_lock(&queue_mutex);

    if (client_head){
        client = client_head;
        client_head = client->next;
        if (client_head == NULL){
            client_tail = NULL;
        }
        fd = client->fd;
        free(client);
    }
    else{
        client = NULL;
        fd = -1; 
    }

    pthread_mutex_unlock(&queue_mutex);
    return fd; 
}

/* Function run by each handling thread to service clients */ 
/* Consuming function - retrieves from the queue and handles requests" */
void *handler_loop( void* data ){
    int menu; // integer to store select menu option from client.
    Game game;
    connection_t *client = &thread_info[*(int*)data];
    client->thread = *(int*)data;
    free(data);
    client->connect = false; 

    printf("Thread %d: ready for clients...\n", client->thread);

    while(server_running){ 
        sem_wait(&full); // wait for client to enter queue
        if (!server_running) break;
        client->fd = get_next_client(); // retrieve client socket from queue
        client->connect = true; // establish connect while client is authenticated. 

        printf("thread %d: handling socket %d\n", client->thread, client->fd);

        // send start signal to client 
        printf("Thread %d: sending start command to socket %d\n", client->thread,client->fd);
        
        send_sock(client->fd, START_COMMAND, COMMAND_SIZE);

        // authenticate client 
        printf("Thread %d: Authenticating client at socket %d...\n", client->thread, client->fd);

        // if client authorised label as connected and initialise the game. 
        if (get_login_info(client) != CLOSED){
            int val = authenticated(client);
            if (val == 1){
                printf("Thread %d: %s on socket %d authenticated\n", client->thread, client->username, client->fd);
                send_sock(client->fd, AUTHENTICATED, COMMAND_SIZE);
            }
            else{
                printf("thread %d: Client entered wrong username or password. Disconnecting.\n",
                client->thread);
                send_sock(client->fd, WRONG_PASS, COMMAND_SIZE);
                client->connect = false; // disconnect client 
            }
        }

        // Initialise client's game 
        while(client->connect){
            if (!server_running) break;

            menu = client_menu_select(client);
            switch(menu){
                case 1: // play_game
                    printf("Thread %d: %s entering game loop\n", client->thread, client->username);
                    play_game_inst(client, &game);   // enter game loop 
                    break;
                case 2: // recieve leaderboard
                    printf("Thread %d: sending leaderboard to %d\n", client->thread, client->fd);
                    send_leaderboard(client->fd);
                    break;
                case 3: // disconnect 
                    printf("Thread %d: disconnect client %d\n", client->thread, client->fd );
                    client->connect = false;  // stop client handling loop
                    break;
            }
        }
        disconnect_thread(client); // disconnect client 
        sem_post(&vacant);
    }
    return NULL;
}

// function to recieve login information from a client.

int get_login_info(connection_t *thread){
    if (rec_sock(thread->fd, (void *)thread->username, MAX_SIZE) == -1){
        // client disconnected.
        thread->connect = false;
        return CLOSED;
    }
    if (rec_sock(thread->fd, (void *)thread->password, MAX_SIZE) == -1){
        // client disconnected 
        thread->connect = false; 
        return CLOSED; 
    }
    return 1; 
}

// function to authenticate the client being handled by a thread.
int authenticated(connection_t *thread){

    printf("test\n");
    FILE * file; 
    if (!(file = fopen("server/Authentication.txt", "r"))){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // store usernames and passwords read from the file.
    char f_user[40];
    char f_pass[40];

    fscanf(file, "%s %s", f_user, f_pass); // scan titles to move past

    while(true){
        if(fscanf(file, "%s %s", f_user, f_pass)!=2){
            return 0;
        }
        if ((strcmp(f_user, thread->username)==0)&&(strcmp(f_pass, thread->password)==0)){
            // write zeroes to password to remove data from memory
            bzero(thread->password, strlen(thread->password));
            return 1; 
        }
    }
}

/*============================================*
 *Functions for communicating with the clients*
 *============================================*/

int client_menu_select(connection_t *thread){
    char buf[COMMAND_SIZE];
    if ((rec_sock(thread->fd, buf, MAX_SIZE)) == -1) {
        // client disconnect
		thread->connect = false; 
        return 0; 
	}
    return (int) buf[0];
}

// function to play an instance of a game based on client inputs. 
void play_game_inst(connection_t *thread, Game* game){
    int menu;
    game_init(game); // initialise game for the client.
    while(!game->game_over){
        if (!server_running) break;
        menu = client_menu_select(thread);
        switch(menu){
            case 'R':
                printf("Thread %d: %s revealing tile\n", thread->thread, thread->username);
                if (reveal_tile_user(game, thread->fd) == CLOSED){
                    thread->connect = false; 
                }
                break;
            case 'P':
                printf("Thread %d: %s placing flag\n", thread->thread, thread->username);
                if (flag_tile_user(game, thread->fd) == CLOSED){
                    thread->connect = false; 
                }
                break;
            case 'Q':
                printf("Thread %d: %s quitting game\n", thread->thread, thread->username);
                game->game_over = true; // set game over to true on server side (an unfished game is still noted as a game played)
                break;
        }
    }
    if (server_running){
        time_t elapsed_time = get_elapsed_time(game);
        // check if game was won and update leaderboard. 
        if (game->remaining_mines == 0){
            send(thread->fd, (void *)&elapsed_time, sizeof(time_t), 0);
            printf("Thread %d: %s won the game!\n", thread->thread, thread->username);
        }
        else{
            printf("Thread %d: %s did not win the game!\n", thread->thread, thread->username);
        }

        printf("Thread %d: updating leaderboard.\n", thread->thread);
        // update leaderboard.
        update_leaderboard(thread->username, game_won(game), elapsed_time);
    }

    // free game.
}

/*=======================================================*
 *Functions for closing connections and freeing resources*
 *=======================================================*/

/* Disconnects clients from the server */
void disconnect_thread(connection_t *thread){
    printf("thread %d: disconnected\n", thread->thread);
    thread->connect = false; 
    send_sock(thread->fd, QUIT_COMMAND, COMMAND_SIZE);
    close(thread->fd);
    shutdown(thread->fd, 0);
    thread->fd = -1;
}

/* Shut down server */ 
void close_server( int var ){
    int i;
    system("clear");
    printf("shutting down server.\n");
    server_running = false; 

    for (i = 0; i < MAX_CLIENTS; i++){
        sem_post(&full);
        if (thread_info[i].connect){
            printf("thread %d, sock %d\n", thread_info[i].thread, thread_info[i].fd);
            //send_sock(thread_info[i].fd, QUIT_COMMAND, COMMAND_SIZE);
            disconnect_thread(&thread_info[i]);
        }
    }

    close(server_socket);
    shutdown(server_socket, 0);

    for(i = 0; i < MAX_CLIENTS; i++){
        pthread_cancel(c_threads[i]);
    } 

    free_resources();
    exit(EXIT_SUCCESS);
}

/* Free allcoated memory */ 
void free_resources( void ){
    while(get_next_client() != -1){
        // releasing resources of client queue
    }
    cleanup_leaderboard_data();
}

