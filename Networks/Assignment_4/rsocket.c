#include "rsocket.h"


void *thread_R(){

}
void *thread_S(){

}

int r_socket(int domain, int type, int protocol){
    sockfd = socket(domain, type, protocol);
    
    //initialize tables


    
    if (pthread_mutex_init(&ulock, NULL) != 0) {
        printf("\nError in mutex initialization\n");
        exit(0);
    }if (pthread_mutex_init(&rlock, NULL) != 0) {
        printf("\nError in mutex initialization\n");
        exit(0);
    }
	pthread_create(&R, NULL, thread_R, NULL);
    pthread_create(&S, NULL, thread_S, NULL);

    return sockfd;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return bind(sockfd, addr, addrlen);
}
