#include <stdio.h> 
#include <string.h>    
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>

#define PORT 8888
#define MAX_CLIENTS 3 
#define MAX_BUFF 1025
     
typedef struct client{
    char ip[MAX_BUFF];
    int port;
} Client;


int main(int argc , char *argv[])  
{  
    int opt = 1;
    Client clientId[MAX_CLIENTS];
    int master_socket, addrlen, new_socket, clientsockfd[MAX_CLIENTS], numclient = 0;
    int activity, i, valread, sd;  
    int max_sd;  
    struct sockaddr_in address;  
                  
    fd_set readfds;  

    for (i=0;i<MAX_CLIENTS;i++){
        clientsockfd[i] = -1;
    }

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }
     
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
         
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
         
    if (listen(master_socket, MAX_CLIENTS) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    addrlen = sizeof(address);  
    printf("Waiting for connections ...\n");  
         
    while(1)  
    {  
        FD_ZERO(&readfds);  
     
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        for ( i = 0 ; i < MAX_CLIENTS ; i++)  
        {  
            sd = clientsockfd[i]; 
            if(sd > 0) FD_SET(sd , &readfds);  
            if(sd > max_sd) max_sd = sd;  
        }  

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);  
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            
            if (numclient >= MAX_CLIENTS){
                perror("Client limit reached");
                exit(EXIT_FAILURE);
            }

            clientsockfd[numclient] = new_socket;
            Client c;
            strcpy(c.ip, inet_ntoa(address.sin_addr));
            c.port = ntohs(address.sin_port);
            clientId[numclient] = c;
            numclient++;
            printf("Server: Received a new connection from client <%s: %d>\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));  

        }  
        for (i = 0; i < MAX_CLIENTS; i++)  
        {  
            sd = clientsockfd[i];  
            char buffer[MAX_BUFF];
                 
            if (FD_ISSET(sd, &readfds))  
            {  
                if ((valread = read(sd, buffer, 1024)) == 0)  
                {
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    close(sd);  
                    clientsockfd[i] = 0;  
                }
                else{
                    buffer[valread] = '\0';  
                    printf("Server: Received message “%s” from client <%s: %d>\n", buffer, clientId[i].ip, clientId[i].port);
                    if (numclient > 1){
                        char message[MAX_BUFF];
                        strcpy(message, clientId[i].ip);
                        char port_str[2];
                        sprintf(port_str, "%d", clientId[i].port);
                        strcat(message, port_str);
                        strcat(message, buffer);
                        for (int clId = 0; clId < numclient; clId++){
                            if (clId != i){
                                send(clientsockfd[clId], message, strlen(message), 0);
                                printf("Server: Sent message “%s” from client <%s: %d> to <%s: %d>\n", buffer, clientId[i].ip, clientId[i].port, clientId[clId].ip, clientId[clId].port);
                            }   
                        }
                    }
                    else{
                        printf("Server: Insufficient clients, “%s” from client <%s: %d> dropped\n", buffer, clientId[i].ip, clientId[i].port);
                    }
                }  
            }  
        }  
    }  
         
    return 0;  
}  