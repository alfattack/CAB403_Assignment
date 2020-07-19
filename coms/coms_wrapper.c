
#include "coms_wrapper.h"

int rec_sock(int sock_fd, void * buf, int size){
    int num_bytes = recv(sock_fd, buf, size, 0);

    if (num_bytes == -1){
        perror("recv");
        exit(EXIT_FAILURE);
    }
    
    // recieved quit command.
    else if (strcmp(buf, QUIT_COMMAND) == 0){
        return -1;
    }
    else{
        return 0;
    }
}

int send_sock(int sock_fd, void * buf, int size){

    if (send(sock_fd, buf, size, 0) == -1){
        perror("send");
        exit(EXIT_FAILURE);
    }
    return 0;
}
