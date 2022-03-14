#include "rsocket.h"
#define PORT 50121


// Driver code
int main() {
    int sockfd;
    struct sockaddr_in   servaddr,cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    int n, len;

    len = sizeof(servaddr);

    char buf[100];
    n = recvfrom(sockfd, buf, sizeof(buf),MSG_WAITALL, ( struct sockaddr *) &cliaddr,&len);
    printf("Client : %s\n",buf);   
    // n = recvfrom(sockfd, &mm, sizeof(mm),MSG_WAITALL, (struct sockaddr *) &servaddr,&len);
    // printf("Server : %s\n", mm.buf);

    close(sockfd);
    return 0;
}
