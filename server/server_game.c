#include "server_game.h"

/*global variables*/
pthread_mutex_t rand_mutex;

// initialise a game object for server.
int game_init(Game *game){
    int i, j; 
    for(i = 0; i < NUM_TILES_X; i++){
        for(j = 0; j < NUM_TILES_Y; j++){
            game->tiles[i][j].adjacent_mines = 0;
            game->tiles[i][j].is_bomb = 0;
            game->tiles[i][j].is_revealed = 0; 
            game->tiles[i][j].x = i;
            game->tiles[i][j].y = j;
            game->tiles[i][j].flagged = 0; 
            game->start_time = time(NULL);
        }
    }
    printf("Size of tile %ld\tSize of flag attr: %ld\n", sizeof(Tile), sizeof(game->tiles[0][0].flagged));

    game->game_over = false;
    game->remaining_mines = NUM_MINES;
    place_mines(game); 
    return 1; 
}

time_t get_elapsed_time(Game *game){
    return time(NULL) - game->start_time;
}

int tile_is_mine(Game *game, int x, int y){
    return game->tiles[x][y].is_bomb;
}

void place_mines(Game *game){
    int x, y;
    for (int i = 0; i < NUM_MINES; i++){
        do{
            pthread_mutex_lock(&rand_mutex);
            x = rand() % 9; 
            y = rand() % 9; 
            pthread_mutex_unlock(&rand_mutex);
        } while (tile_is_mine(game, x, y));
        game->tiles[x][y].is_bomb = 1;

        // update adjacent_mines for neighbours  
        for (int x_next = -1; x_next < 2; x_next++ ){
            // skip if neighbour x value is out of grid range
            if ((x + x_next == -1)||(x + x_next == NUM_TILES_X))
                continue; 

            for (int y_next = -1; y_next < 2; y_next++){
                // skip if neighbour y value is out of grid range
                if ((y + y_next == -1)||(y + y_next == NUM_TILES_Y))
                    continue;
                game->tiles[x+x_next][y+y_next].adjacent_mines++;
            }
        }
    }
}

// get supplied coordinates from client on a socket (validity is handled on client's end)
int get_coords(int sock_fd, char * buf){
    if (rec_sock(sock_fd, (void *)buf, 3) == -1){
        return -2;
    }
    else{
        return 1;
    }
}

// when user chooses to reveal a tile.
int reveal_tile(Game* game, int x, int y, int sock_fd){
    // preemptively return with -1 if tile has already been revealed  
    if (game->tiles[x][y].is_revealed){
        return -1; // error - tile is already revealed. 
    }

    // reveal tile
    game->tiles[x][y].is_revealed = 1;
    // send tile 
    send_tile(sock_fd, &game->tiles[x][y]);

    // return 1 if the tile is a bomb - reveal the board
    if (tile_is_mine(game, x, y)){
        game->game_over = true;
        reveal_all_mines(game, sock_fd);
        //update_board(game);
        return 1; // return 1 as tile was mine
    } 
    // if tile has 0 surrounding mines continue to reveal neighbouring tiles
    if (game->tiles[x][y].adjacent_mines == 0){
        for (int x_next = -1; x_next < 2; x_next++ ){
            // skip if neighbour x value is out of grid range
            if ((x + x_next == -1)||(x + x_next == NUM_TILES_X))
                continue; 

            for (int y_next = -1; y_next < 2; y_next++){
                // skip if neighbour y value is out of grid range
                if ((y + y_next == -1)||(y + y_next == NUM_TILES_Y))
                    continue;

                if ((x_next == x)&&(y_next == y)){
                    continue; 
                }
                reveal_tile(game, x+x_next, y+y_next, sock_fd);
            }
        }
        return 0;
    }
    return 0; // return 0 as tile was not mine.
}

int reveal_all_mines(Game *game, int sock_fd){
    for(int x = 0; x < NUM_TILES_X; x++){
        for (int y = 0; y < NUM_TILES_Y; y++){
            if ((game->tiles[x][y].is_revealed)||(!game->tiles[x][y].is_bomb)){
                continue;
            }
            else{
                game->tiles[x][y].is_revealed = 1; 
                send_tile(sock_fd, &game->tiles[x][y]);
            }
        }
    }
    return 1; 
}

int reveal_tile_user(Game *game, int sock_fd){
    char coords[3];
    int x, y;
    get_coords(sock_fd, coords);

    x = coords[1]-49;
    y = coords[0]-65;

    int returnVal = reveal_tile(game, x, y, sock_fd);

    if (returnVal == 1){
        send_sock(sock_fd, WAS_MINE, COMMAND_SIZE); // revealed tile was mine.
    }
    else if (returnVal == 0){
        send_sock(sock_fd, ALL_TILES, COMMAND_SIZE); // revealed tile was not mine.
    }
    else{
        send_sock(sock_fd, ALREADY_REVEAL, COMMAND_SIZE); // already revealed.
    }
    return returnVal;
}

// function to flag a tile based on user supplied coordinates. 
// Sends message to signify if tile was a bomb, not a bomb, or already revealed

int flag_tile_user(Game *game, int sock_fd){
    char coords[3];
    int x, y;
    int val; // store value from send_tile function.
    get_coords(sock_fd, coords);

    x = coords[1]-49; // convert ascii supplied coordinates to integers. 
    y = coords[0]-65;

    if (game->tiles[x][y].is_revealed){
        send_sock(sock_fd, ALREADY_REVEAL, COMMAND_SIZE); // if tile already revealed don't send any tile. 
        val = -1; // error: tile already revealed. 
    }
    else{
        game->tiles[x][y].flagged = 1; // flag and reveal tile.
        game->tiles[x][y].is_revealed = 1;

        val = send_tile(sock_fd, &game->tiles[x][y]); // send tile.

        if (val){ // tile was mine. 
            game->remaining_mines -= 1;
            if (game->remaining_mines == 0){
                game->game_over = true;
            }
            send_sock(sock_fd, WAS_MINE, COMMAND_SIZE);
        }
        else{ // tile was not mine.
            send_sock(sock_fd, ALL_TILES, COMMAND_SIZE);
        }
    }
    return val;    
}


int send_tile(int sock_fd, Tile * tile){
    char buf[COMMAND_SIZE];
    send(sock_fd, (void *)tile, sizeof(Tile), 0 );
    do{
        if (rec_sock(sock_fd, buf, COMMAND_SIZE) == -2){
            return -2; 
        }
    } while (strcmp(buf, RECIEVED));

    if (tile->is_bomb){
        return 1; // tile is bomb
    }
    else{
        return 0; // tile not bomb
    }
}

int game_won(Game * game){
    if (game->remaining_mines == 0){
        return 1;
    }
    else{
        return 0;
    }
}
