/*********
Name: Shashvat Gupta
Roll Number: 19CS30042
*********/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8888
#define MAXDATASIZE 100

int main()
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct sockaddr_in serv_addr; 

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1){
        perror("calling fcntl");
        exit(EXIT_FAILURE);
    }

    int N=1;
    char ip[5],Port[3];
    
    while (1){
        sleep(rand()%3+1);
        memset(buf,'\0',MAXDATASIZE);
        if(N<6){
            sprintf(buf,"Message <%d>",N);
            send(sockfd, buf, MAXDATASIZE, 0);
            printf("Message %d sent\n",N);
            N++;
        }
        memset(buf, '\0', MAXDATASIZE);
        int bytes_recv = recv(sockfd, buf, MAXDATASIZE, 0);
        if(bytes_recv<=0){
            continue;
        }
        int start=0;
        while(bytes_recv>start){
            memset(ip,'\0',5);
            memset(Port,'\0',3);
            for(int i=0;i<4;i++,start++){
                ip[i]=buf[start];
            }
            for(int i=0;i<2;i++,start++){
                Port[i]=buf[start];
            }
            printf("Client: Received ");
            for(;;start++){
                if(buf[start]=='\0'){
                    start++;
                    break;
                }
                printf("%c",buf[start]);
            }
            printf(" from %s:%s",ip,Port);
        }
        
    }

    close(sockfd);

    return 0;
}
