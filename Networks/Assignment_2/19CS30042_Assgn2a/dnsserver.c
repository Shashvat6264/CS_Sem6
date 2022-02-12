// A Simple UDP Server that sends a HELLO message
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
  
#define MAX_BUFF 100
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("Server Running....\n");
  
    int n; 
    socklen_t len;
    char buffer[MAX_BUFF], dnsName[MAX_BUFF]; 
    len = sizeof(cliaddr);
    struct hostent *hp;
    struct in_addr **ip_addr_list;

    while (1){
        memset(buffer, '\0', MAX_BUFF);
        memset(dnsName, '\0', MAX_BUFF);

        n = recvfrom(sockfd, (char *)buffer, MAX_BUFF, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';
        printf("DNS Name received from client: ");
        printf("%s\n", buffer);
        strcpy(dnsName, buffer);
        
        hp = gethostbyname(dnsName);
        if (!hp){
            printf("%s DNS not found\n", dnsName);
            sendto(sockfd, "0.0.0.0\0", strlen("0.0.0.0\0"), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
        }
        else{
            ip_addr_list = (struct in_addr **)(hp->h_addr_list);
            for (int i=0;ip_addr_list[i]!=NULL;i++){
                printf("DNS Name: %s found to IP: %s\n", dnsName, inet_ntoa(*ip_addr_list[i]));
                memset(buffer, '\0', MAX_BUFF);
                strcpy(buffer, inet_ntoa(*ip_addr_list[i]));
                sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
        sendto(sockfd, "*\0", strlen("*\0"), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
    }
      
    return 0; 
} 