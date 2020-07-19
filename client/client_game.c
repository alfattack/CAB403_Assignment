#include "client_game.h"

// initialising game for client (all tiles unknown at start)
void game_init(Game * game){
    int i, j; 
    for(i = 0; i < NUM_TILES_X; i++){
        for(j = 0; j < NUM_TILES_Y; j++){
            game->tiles[i][j].is_revealed = 0;
            game->tiles[i][j].flagged = 0;  
        }
    }
    game->game_over = false;
    game->remaining_mines = NUM_MINES;
}

// free memory allocated to game. 

// send input coordinates to server.
void send_coordinates(int sock_fd){
    int x, y; 

    while(1){
        printf("Enter co-ordinates: ");
        char c, coords[MAX_SIZE];
        scanf("%s", coords);
        x = coords[1]-49;
        y = coords[0]-65;
        
        // check if input values are valid.
        if ((strlen(coords)!=2)||(x>9)||(x<0)||(y>9)||(y<0)){ 
            printf("Invalid coordinates.\n");
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        } 
        else{
            send_sock(sock_fd, coords, 3);
            break;
        }
    }
}

// Function to continually recieve tiles until the server says there are no more. 
int recieve_tiles(int sock_fd, Game * game){
    char buf[MAX_SIZE];
    Tile rec_tile;
    int x, y;
    while(1){
        if (rec_sock(sock_fd, buf, MAX_SIZE) == -1){
            printf("Server closed\n");
            return CLOSED; 
            exit(1);
        }
        if (strcmp(buf, ALL_TILES) == 0){
            return 0; // was not mine.
        }
        else if (strcmp(buf, WAS_MINE) == 0){
            return 1; // tile was mine.
        }
        else if (strcmp(buf, ALREADY_REVEAL) == 0){
            return -1; // tile already revealed.
        }
        else{
            rec_tile = *(Tile*)buf; 
            x = rec_tile.x;
            y = rec_tile.y;
            game->tiles[x][y]=rec_tile;
        }
        send_sock(sock_fd, RECIEVED, COMMAND_SIZE);
    }
}

void print_board(Game *game){
    char buf[MAX_SIZE];
    int bufIndex = 0; 
    printf("Remaining mines: %d\n\n", game->remaining_mines);
    char cols[] = "    1 2 3 4 5 6 7 8 9\n"
                  "---------------------\n";

    for(; bufIndex < 44; bufIndex++){
        buf[bufIndex] = cols[bufIndex];
    }

    for(int y = 0; y < 9; y++){
        buf[bufIndex++] = 'A' + y; 
        buf[bufIndex++] = ' ';
        buf[bufIndex++] = '|';
        buf[bufIndex++] = ' ';
        for(int x = 0; x < 9; x++){
            if ((game->tiles[x][y].flagged)&&(!game->game_over)){
                buf[bufIndex++] = (game->game_over)? ' ' : '+';
                buf[bufIndex++] = ' ';
            }
            else if (!game->tiles[x][y].is_revealed){
                buf[bufIndex++] = ' ';
                buf[bufIndex++] = ' ';
            }
            else if (game->tiles[x][y].is_bomb){
                buf[bufIndex++] = '*';
                buf[bufIndex++] = ' ';
            }
            else{
                buf[bufIndex++] = game->tiles[x][y].adjacent_mines + '0';
                buf[bufIndex++] = ' ';
            }
        }
        buf[bufIndex++] = '\n';
    }
    buf[bufIndex] = '\0';
    printf("%s",buf);
}

bool game_won(Game * game){
	if (game->remaining_mines == 0) game->game_over = true; // set game over to true (may already be set to true.)
	return (game->remaining_mines == 0);
}