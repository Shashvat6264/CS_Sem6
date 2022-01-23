// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <signal.h>

#define MAX_BUFF 100

void sig_handler(int signum){
    printf("Error: Response from server taking too long\n");
    exit(EXIT_FAILURE);
}
  
int main() { 
    char dnsName[MAX_BUFF], buffer[MAX_BUFF];
    printf("Enter DNS name: ");
    scanf("%[^\n]%*c", dnsName);

    signal(SIGALRM, sig_handler);

    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
      
    sendto(sockfd, (const char *)dnsName, strlen(dnsName), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    printf("DNS Name sent to server\n");
    alarm(2);
    
    n = recvfrom(sockfd, (char *)buffer, MAX_BUFF, 0, (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';
    if (strcmp(buffer, "0.0.0.0\0") == 0) printf("No IP found for this DNS Name\n");
    else if (strcmp(buffer, "*\0") != 0) printf("IP Received from server: %s\n", buffer);
    while (strcmp(buffer, "*\0") != 0){
        memset(buffer, '\0', MAX_BUFF);
        n = recvfrom(sockfd, (char *)buffer, MAX_BUFF, 0, (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';
        if (strcmp(buffer, "*\0") != 0) printf("IP Received from server: %s\n", buffer);
    }
    close(sockfd); 
    return 0; 
} 