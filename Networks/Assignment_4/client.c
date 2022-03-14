// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT     8080
#define MAXLINE 1024

struct msg{
    int sequence;
    char buf[100];
};

// Driver code
int main() {
    int sockfd;
    struct msg mm;
    //char buffer[MAXLINE];
    //char *hello = "Hello from client";
    struct sockaddr_in   servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    
    int n, len;
    

    mm.sequence = 1;
    strcpy(mm.buf,"hello sent");
    sendto(sockfd, &mm, sizeof(mm),
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,
            sizeof(servaddr));
    printf("Hello message sent.\n");
        
    n = recvfrom(sockfd, &mm, sizeof(mm),
                MSG_WAITALL, (struct sockaddr *) &servaddr,
                &len);
    //buffer[n] = '\0';
    printf("Server : %s\n", mm.buf);

    close(sockfd);
    return 0;
}
