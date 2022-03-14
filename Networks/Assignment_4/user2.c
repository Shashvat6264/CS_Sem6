#include "rsocket.h"
#define PORT 50121


// Driver code
int main() {
    int M2;
    struct sockaddr_in servaddr,cliaddr;

    // Creating socket file descriptor
    if ( (M2 = r_socket(AF_INET, SOCK_MRP, 0)) < 0 ) {
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
    if ( r_bind(M2, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        r_close(M2);
        exit(EXIT_FAILURE);
    }
    
    int n;
    socklen_t len;

    len = sizeof(servaddr);

    char buf[100];
    while((n = r_recvfrom(M2, buf, sizeof(buf), ( struct sockaddr *) &cliaddr,len))>0){
        printf("Client : %s\n",buf);  
        n = 0; 
    }
    // n = recvfrom(M2, &mm, sizeof(mm),MSG_WAITALL, (struct sockaddr *) &servaddr,&len);
    // printf("Server : %s\n", mm.buf);

    r_close(M2);
    return 0;
}
