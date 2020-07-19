/* CAB403 Client Program *
 * Alexander Brimblecome *
 * N10009833             */

#include "client.h"

/* Global variables */ 
int sock_fd;
char command[COMMAND_SIZE];
Game game; 
bool alive; // boolean for whether the server is currently running 

/* Main function */ 
int main(int argc, char** argv){
	signal(SIGINT, disconnect_from_server);

    if (argc != 3) {
		fprintf(stderr,"expected: client_hostname port_number\n");
		exit(1);
	}
	// connect to server...
    connect_to_server(argv[1], argv[2]);

	// wait for server to allocate resources to the client. 
	printf("waiting for server...\n");
	while(strcmp(command, START_COMMAND)){
		if (rec_sock(sock_fd, command, COMMAND_SIZE) == -1){
			alive = false; 
			exit(EXIT_SUCCESS);
		}
	}
	printf("Server now ready for client...\n");

	system("clear");

	printf("================================================\n");
	printf("Welcome to the minesweeper online gaming system!\n");
	printf("================================================\n\n");

	if (!authenticate()){
		printf("\n\nUsername or password was incorrect\n\n");
		exit(1);
	}
	else{
		system("clear");
		game_init(&game);
		printf("Welcome to the minesweeper online gaming system!\n\n");
	}

	int menu_selection;

	while((menu_selection != 3)&&(alive)){
		flush_input();
		printf("<1> play game.\n<2> show leaderboard\n<3> quit.\n");
		scanf("%d", &menu_selection);
		send(sock_fd, (void*)&menu_selection, 4, 0);

		switch(menu_selection){
			case 1:
				play_game();
				break;
			case 2:
				get_leaderboard();
				break;
		}
	}

	printf("Disconnecting\n");
	disconnect_from_server();
    return 0; 
}

// function to authenticate user by sending username and password to server.
bool authenticate(){
	char username[MAX_SIZE];
	char password[MAX_SIZE];
	char response[COMMAND_SIZE];
	printf("You are required to log on with your username and password\n\n");

	printf("Username: ");
	scanf("%s", username);
	send(sock_fd, (void*)username, strlen(username)+1, 0); // send username to server
	printf("Password: ");
	scanf("%s", password);
	send(sock_fd, (void*)password, strlen(password)+1, 0 ); // send password to server

	rec_sock(sock_fd, response, COMMAND_SIZE); // response from server

	if (strcmp(response, AUTHENTICATED)){
		return false;
	}
	else{
		return true; 
	}
}

void flush_input(){
	char c; 
	while ((c = getchar()) != '\n' && c != EOF) { }
}

// function to enter gameplay loop. 
void play_game(){
	char menu_selection;
	char c; // dummy char used for waiting until user presses enter. 
	time_t elapsed_time; 

	game_init(&game); // initialise game.

	while(!game.game_over){
		if (!alive) break; 
		flush_input();
		system("clear");
		print_board(&game);

		printf("\nChoose an option:\n<R> Reveal Tile\n<P> Place flag\n<Q> Quit game\n");
		scanf("%c", &menu_selection);

		send(sock_fd, (void*)&menu_selection, 4, 0);

		switch(menu_selection){
			case 'R':
				reveal_tile_option(sock_fd, &game);
				break;
			case 'P':
				flag_tile_option(sock_fd, &game);
				break;
			case 'Q':
				system("clear");
				return;  // if client quits game - return from function.
		}
	}
	// check game conditions to see if client won / lost and update leaderboard.

	// get time elapsed since start of game. 

	if (game_won(&game)){
		
		// recieved elapsed time from server.
		if (rec_sock(sock_fd, (void *)&elapsed_time, sizeof(timer_t)) == 1){
			alive = false;
			printf("server disconnected\n");
			return;
		}

		system("clear");
		print_board(&game);
		printf("Congratulations! you have located all the mines.\nYou won in %ld seconds!\n", elapsed_time);
	}
	else{
		print_board(&game);
		printf("Game over! you hit a mine.\n");
	}
	scanf("%c", &c); // wait until user presses enter to continue.
}

// gameplay options 

void reveal_tile_option(int sock_fd, Game * game){
	int val; // store value of recieve_tiles func.
	char c; // dummy char to wait for user to press enter with scanf func.
	send_coordinates(sock_fd);

	val = recieve_tiles(sock_fd, game);

	if (val == -1){ // tile already revealed.
		system("clear");
		print_board(game);
		printf("Error: Tile already revealed\n");
		scanf("%c", &c);
	}

	if (val == 1){ // game over
		game->game_over = true;
		system("clear");
	}
}

void flag_tile_option(int sock_fd, Game * game){
	int val; // store value of recieve_tiles func.
	char c; // dummy char to wait for user to press enter with scanf func.
	send_coordinates(sock_fd);
	val = recieve_tiles(sock_fd, game);

	if (val == CLOSED){ // server shut down.
		alive = false;
	}
	else if (val == -1){ // tile already revealed.
		system("clear");
		print_board(game);
		printf("Error: Tile already revealed.\n");
		scanf("%c", &c);
	}
	else if (val == 1){ // tile was mine (successful).
		game->remaining_mines -= 1; 
		game_won(game); // test if game is won. 
	}
	else{ // tile was not mine.
		system("clear");
		print_board(game);
		printf("Not a mine!\nEnter to continue..\n");
		scanf("%c", &c);
	}
}

int get_leaderboard(){
	system("clear");
	char buf[MAX_SIZE];
	printf("=============================================================\n\n");

	while(1){
		if (rec_sock(sock_fd, buf, MAX_SIZE) == -1){
			// server disconnected 
			printf("Server Disconnected...\n");
			alive = false; 
            return -1;
        }

		if (strcmp(buf, ALL_ENTRIES)==0){
			printf("\n=============================================================\n");
			break;
		}
		else{
			printf("%s",buf);
			send_sock(sock_fd, RECIEVED, COMMAND_SIZE);
		}
	}
	return 0;
}

/* Function to connect to the server */
void connect_to_server(char* ip, char* port){
    struct  hostent *he; 
    struct  sockaddr_in their_addr;

     if ((he=gethostbyname(ip)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

    their_addr.sin_family = AF_INET; 
	their_addr.sin_port = htons(atoi(port));   
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);

    if (connect(sock_fd, (struct sockaddr *)&their_addr, \
	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	alive = true; 
}

/* Function to connect to the server */
void disconnect_from_server( void ){
	system("clear");
	printf("Disconnecting\n");
	if (alive){
		send_sock(sock_fd, QUIT_COMMAND, COMMAND_SIZE);
	}
	close(sock_fd);
	exit(EXIT_SUCCESS);
}



