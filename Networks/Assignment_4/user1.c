#include "rsocket.h"

#define PORT     50120
#define MAXLINE 1024

// Driver code
int main() {
    int M1;
    struct sockaddr_in servaddr, cliaddr;
    
    // Creating socket file descriptor
    if ( (M1 = r_socket(AF_INET, SOCK_MRP, 0)) < 0 ) {
        perror("socket creation failed");
        r_close(M1);
        exit(EXIT_FAILURE);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    // Filling user1 information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
     // Filling user2 information
    cliaddr.sin_family = AF_INET; // IPv4
    // cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_port = htons(PORT+1);
    inet_aton("127.0.0.1", &cliaddr.sin_addr); 

    // Bind the socket with the server address
    if ( r_bind(M1, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    int  n;

    // len = sizeof(cliaddr); //len is value/resuslt

    char buf[100];
    printf("Enter a string (of 25<size<50): \n");
    scanf("%[^\n]s",buf);
    for (int i = 0; i < strlen(buf); ++i){
        char ch = buf[i];
        if((n=r_sendto(M1, &ch, 1, (const struct sockaddr *) &cliaddr,sizeof(cliaddr)))<0){
            printf("Error in sending msg\n");
        }
    }
    
    r_close(M1);
    return 0;
}
