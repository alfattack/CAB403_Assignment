#include <stdio.h> 
#include <stdbool.h> 
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include "../coms/coms_wrapper.h"
#include "../coms/commands.h"

typedef struct user_profile{
    char username[40];
    int games_played;
    int games_won; 
    struct user_profile * next; 
}profile;

typedef struct leader_node{
    profile * player;
    time_t time_elapsed;
    struct leader_node * next; 
}node; 


int update_leaderboard(char * username, int won, int elapsed_time);
void leaderboard_insert(node * entry);
void insert_entry(node * current, node * previous, node * entry);
int sort_leaderboard( );
void swap_entries(node * prev, node * cur, node * next);
void remove_entry(node * entry);
bool check_alphabet(char * name_one, char * name_two);
profile * update_profile(char * username, int won);
void free_profiles( );
void free_leaderboard( );
void cleanup_leaderboard_data( );
int send_leaderboard(int sock_fd);
