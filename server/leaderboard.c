#include "leaderboard.h"

node * leader_head = NULL; // linked list for storing leaderboard entries. 

profile * profile_head = NULL; //linked list for storing profile entries 

/* Read and Write lock from CAB403 Tutorial 5*/
int rc;	/* readcount */
pthread_mutex_t rc_mutex;
pthread_mutex_t r_mutex;
pthread_mutex_t w_mutex;

void ReadLock( ){
    pthread_mutex_lock( &r_mutex );
	pthread_mutex_lock( &rc_mutex );
	rc++;
	if ( rc == 1 ) pthread_mutex_lock( &w_mutex );
	pthread_mutex_unlock( &rc_mutex );
	pthread_mutex_unlock( &r_mutex );
}

void ReadUnlock( ){
    pthread_mutex_lock( &rc_mutex );
	rc--;
	if (rc == 0) pthread_mutex_unlock( &w_mutex );
	pthread_mutex_unlock( &rc_mutex );
}

void WriteLock( ){
    pthread_mutex_lock(&r_mutex);
	pthread_mutex_lock(&w_mutex);
}

void WriteUnlock( ){
    pthread_mutex_unlock(&w_mutex);
	pthread_mutex_unlock(&r_mutex);
}

/******************************************************/

// update the leaderboard list
int update_leaderboard(char * username, int won, int elapsed_time){
    WriteLock();
    profile * player = update_profile(username, won);

    if (!won){
        WriteUnlock();
        return 0;
    } 

    node * entry = malloc(sizeof(node));

    entry->player = player; 
    entry->time_elapsed = elapsed_time; 
    leaderboard_insert(entry);
    while (sort_leaderboard()){
        // continue to sort leaderboard while entries have been swapped.
    }
    WriteUnlock();
    return 1;
}

// Inserts entry into leaderboard in its ordered position.
void leaderboard_insert(node * entry){
    node * current = leader_head;
    node * prev = NULL; 

    if (leader_head == NULL){ // first item in list. 
        leader_head = entry; 
        return;
    }

    // if entry time is greater than the head of the list time - replace as head.
    else{
        bool cont = false; // boolean to unlock if statements. 
        while(current != NULL){
            // if entry time is over or equal to the current node's time - compare attributes. 

            // check elapsed time.
            if (entry->time_elapsed >= current->time_elapsed){
                if (entry->time_elapsed == current->time_elapsed){
                        cont = true; 
                }
                else{
                    insert_entry(current, prev, entry);
                    return;
                }
            }

            // check games won for players in entry and current nodes. 
            if ((entry->player->games_won <= current->player->games_won)&&(cont)){
                insert_entry(current, prev, entry);
                return;
            }
            else if ((entry->player->games_won == current->player->games_won)&&(cont)){
                // do nothing. 
            }
            else{
                cont = false; 
            }

            // check alphabetical ordering.
            if((check_alphabet(current->player->username, entry->player->username))&&(cont)){
                insert_entry(current, prev, entry);
                return;
            }
            prev = current;
            current = prev->next; 
            cont = false; 
        }
    }

    // all entrys in leaderboard equal - add to tail. 
    prev->next = entry; 
    return;
}

void insert_entry(node * current, node * previous, node * entry){
    // first item in list. 
    if (previous == NULL){
        entry->next = leader_head;
        leader_head = entry;
    }
    else{
        previous->next = entry;
        entry->next = current; 
    }
}

// sort the leaderboard based on updated wins for players
int sort_leaderboard(){
    // only one item in list 
    if (!leader_head->next) return 0; 

    node * next = leader_head->next;
    node * cur = leader_head; 
    node * prev; 


    while(next != NULL){
        if((cur->player->games_won > next->player->games_won)&&(cur->time_elapsed == next->time_elapsed)){
            swap_entries(prev, cur, next);
            return 1;
        }
        else if ((cur->player->games_won == next->player->games_won)&&(cur->time_elapsed == next->time_elapsed)){
            if (check_alphabet(cur->player->username, next->player->username)){
                swap_entries(prev, cur, next);
                return 1;
            }
        }
    prev = cur; 
    cur = next;
    next = cur->next;    
    }
    return 0;
}

void swap_entries(node * prev, node * cur, node * next){
    if (prev == NULL){
        leader_head = next; 
        cur->next = next->next; 
        next->next = cur;
    }
    else{
        prev->next = next; 
        cur->next = next->next;
        next->next = cur; 
    }
}

// removes entry from leaderboard. (Doesn't free memory as this function is designed 
// to be used when the entry is taken out, and put back with ordering)
void remove_entry(node * entry){
    if (entry == leader_head){
        leader_head = leader_head->next;
        return;
    }
    node * cur = leader_head->next;
    node * prev = leader_head;  

    while(cur != NULL){
        if (cur == entry){
            prev->next = cur->next;
            return;
        }
        prev = cur;
        cur = prev->next;
    }
}

// compares two strings - returns true if the first string provided comes first alphabetically.
bool check_alphabet(char * name_one, char * name_two){
    int len_one = strlen(name_one);
    int len_two = strlen(name_two);
    int max = (len_one > len_two)? len_two : len_one;

    // compare each letter in the names. 
    for (int i = 0; i < max; i++){
        if (name_one[i] < name_two[i]){
            return true;
        }
        else if(name_one[i] > name_two[i]){
            return false;
        }
    }
    // if words equal up until this point compare lengths (empty spaces come before letters)
    if(strlen(name_one)<strlen(name_two)){
        return true;
    }
    else{
        return false; 
    }
}

// function to update the stored profiles of users who have logged on
// during the server's run time. 

profile * update_profile(char * username, int won){
    profile * currentProf = profile_head; 
    profile * prevProf; 

    if (profile_head == NULL){ // first item in list.
        printf("Leaderboard: Adding first profile to leaderboard - %s\n", username);
        profile_head = malloc(sizeof(profile));
        if (!profile_head){
            perror("malloc");
            exit(EXIT_FAILURE);
        } 
        sprintf(profile_head->username, "%s", username); 
        profile_head->games_played=1;
        profile_head->games_won=won;
        profile_head->next=NULL;
        return profile_head; 
    }

    while(1){
        if (currentProf == NULL){ // user does not exist in list. 
            printf("Leaderboard: adding profile for %s\n", username);
            profile * newProfile = malloc(sizeof(profile)); //TO DO: Memory allocation error.
            sprintf(newProfile->username, "%s", username); 
            newProfile->games_played=1;
            newProfile->games_won=won;
            prevProf->next = newProfile;
            return newProfile; 
        }


        else if (strcmp(currentProf->username, username)== 0){ // username found.
            printf("Leaderboard: profile for %s exists in memory\n", username);
            currentProf->games_played++;
            currentProf->games_won+=won;
            return currentProf;
        }

        else{
            prevProf = currentProf;
            currentProf = prevProf->next;
        }
    }
    return NULL;
}

// free memory allocated to profiles. 
void free_profiles(){
    profile * cur = profile_head;
    profile * prev;
    while(cur != NULL){
        prev = cur;
        cur = prev->next;
        free(prev);
    }
}

// free memory allocated to leaderboard.
void free_leaderboard(){
    node * cur = leader_head;
    node * prev;
    while(cur != NULL){
        prev = cur;
        cur = prev->next;
        free(prev);
    }
}

// frees profiles and leaderboards. 
void cleanup_leaderboard_data(){
    WriteLock();
    free_profiles();
    free_leaderboard();
    WriteUnlock();
}

// send leaderboard
int send_leaderboard(int sock_fd){
    ReadLock();
    node * current = leader_head; 
    char buf[MAX_SIZE];

    if (current == NULL){ // no data in leaderboard. 
        send_sock(sock_fd, "\tNo entries in leaderboard.\n", 30);
        do{
            if (rec_sock(sock_fd, buf, COMMAND_SIZE) == -1){
                return CLOSED; 
            }
        } while (strcmp(buf, RECIEVED));
    }    
    else{
        while(current != NULL){
            sprintf(buf, "%s\t\t%ld Seconds\t%d Games won,  %d Games played\n", 
            current->player->username, current->time_elapsed,
            current->player->games_won, current->player->games_played);
            send_sock(sock_fd, buf, MAX_SIZE);
            current = current->next;
            do{
                if (rec_sock(sock_fd, buf, COMMAND_SIZE) == -1){
                    return CLOSED; 
                }
            } while (strcmp(buf, RECIEVED));
        }
    }
    send_sock(sock_fd, ALL_ENTRIES, COMMAND_SIZE);
    ReadUnlock();
    return 1;
}