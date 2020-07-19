/* Types and Constants shared by the client and server in relation 
   to game instances */

#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10
#define MAX_SIZE 256

/* Types used for game instances */

/* Struct for tile type */

typedef struct {
    u_int8_t    adjacent_mines;
    u_int8_t    is_revealed;
    u_int8_t    is_bomb; 
    u_int8_t    flagged;
    u_int8_t    x;
    u_int8_t    y;
}Tile;

/* Struct for game type */

typedef struct {
    bool    game_over;
    Tile    tiles[NUM_TILES_X][NUM_TILES_Y];
    int     remaining_mines;
    int     start_time; 
}Game; 