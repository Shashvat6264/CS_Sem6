#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define PORT 8080
#define MAX_BUFF 200
#define BLK_SIZE 50

// Utility function to print message according to response
int handleResponseCode(char *response_status){
    if (strcmp(response_status, "200") == 0){
        printf("Command executed successfully\n");
        return 1;
    }
    else if (strcmp(response_status, "500") == 0){
        printf("Error code:500 \nError executing command!\n");
        return -1;
    }
    else if (strcmp(response_status, "600") == 0){
        printf("Error code:600 \nIncorrect order of commands!\n");
        return -2;
    }
    else return 0;
}

int put(char*buffer,char *local_file, char *remote_file, int sock){

            int fd;
            if((fd = open(local_file, O_RDONLY))==-1){
                printf("Cannot wite local file\n");
                return -1;
            }
            printf("Sending the entered directory names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory names due to some error\n");
                return -1;
            }
            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            //send(sock,"kk",strlen("kk"),0);
            int status = handleResponseCode(buffer);
            if(status==-1){
                printf("The remote file %s doesn't exist on serevr side \n", remote_file);
                return -1;
            };
            char buffile[BLK_SIZE];
            char temp[BLK_SIZE];
            memset(temp, '\0', BLK_SIZE);
            memset(buffile,'\0',BLK_SIZE);
            short int len = read(fd, buffile, 20);
            sprintf(temp,"M%d",len);    
            strcat(temp,buffile);
            while(1){   
                printf("%d %s\n",len,buffile );
                //memset(buffile,'\0',BLK_SIZE);
                short int len = read(fd, buffile, 20);
                buffile[len] = '\0';
                if(len==0){
                    temp[0] = 'L';
                    if(send(sock, temp, strlen(temp), 0) <0 ){
                        printf("Could Not send response due to some error\n");
                        break;
                    }
                    memset(temp, '\0', BLK_SIZE);
                    recv(sock, temp, 100, 0);
                    printf("%s\n", temp);
                    break;
                }
                if(send(sock, temp, strlen(temp), 0) <0 ){
                    printf("Could Not send response due to some error\n");
                    break;
                }
                memset(temp, '\0', BLK_SIZE);
                recv(sock, temp, 100, 0);
                printf("%s\n", temp);

                memset(temp, '\0', BLK_SIZE);
                sprintf(temp,"M%d",len);    
                strcat(temp,buffile);
            }
            printf("file over!\n");
            close(fd);
}

int main(int argc, char const *argv[]){
	int sock = 0, valread;
	struct sockaddr_in serv_addr;

	// char buffer[MAX_BUFF] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return -1;
    }

    int server_connected = 0;

    while(1){
        printf("myFTP> ");
        char command[MAX_BUFF];
        scanf("%s", command);
        if (strcmp(command, "open") == 0 && !server_connected){
            char IP_address[MAX_BUFF];
            int port;
            scanf("%s", IP_address);
            scanf("%d", &port);
            printf("Executing the command open\n");

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            
            if(inet_pton(AF_INET, IP_address, &serv_addr.sin_addr)<=0){
                printf("\nInvalid address/ Address not supported \n");
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
                printf("\nConnection Failed \n");
                return -1;
            }

            server_connected = 1;
            printf("Connected to server with IP address: %s and PORT: %d\n", IP_address, port);
            continue;
        }
        else if (strcmp(command, "exit") == 0){
            printf("Shutting down the client\n");
            break;
        }
        else if (!server_connected){
            printf("Connect the client to server before running any other command.\nUse open command for the same.\n");
            continue;
        }
        else if (strcmp(command, "user") == 0){
            char username[MAX_BUFF];
            scanf("%s", username);
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "user ");
            strcat(buffer, username);

            printf("Sending the entered username to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the username due to some error\n");
                continue;
            }

            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            int status = handleResponseCode(buffer);
            if (status == -1){
                printf("No such username found\n");
            }
            continue;
        }
        else if (strcmp(command, "pass") == 0){
            char password[MAX_BUFF];
            scanf("%s", password);
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "pass ");
            strcat(buffer, password);

            printf("Sending the entered password to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the password due to some error\n");
                continue;
            }

            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            int status = handleResponseCode(buffer);
            if (status == -1){
                printf("No such user with given username and password found\nTry logging in again\n");
            }
            continue;
        }
        else if (strcmp(command, "cd") == 0){
            char dir_name[MAX_BUFF];
            scanf("%s", dir_name);
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "cd ");
            strcat(buffer, dir_name);

            printf("Sending the entered directory name to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory name due to some error\n");
                continue;
            }

            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            int status = handleResponseCode(buffer);
            if (status == -1){
                printf("Error in shifting to provided directory\n");
            }
            continue;
        }
        else if (strcmp(command, "lcd") == 0){
            char dir_name[MAX_BUFF];
            scanf("%s", dir_name);
            int status = chdir(dir_name) + 1;
            if (status){
                printf("Shifted client to directory %s\n", dir_name);
            }
            else{
                printf("Could not find the path %s\n", dir_name);
            }
            continue;
        }
        else if (strcmp(command, "dir") == 0){
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "dir");

            printf("Sending the entered command dir to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the command due to some error\n");
                continue;
            }

            printf("Following is the list of contents of server directory:\n");
            recv(sock, buffer, MAX_BUFF, 0);
            while (strlen(buffer) != 0){
                char *token = strtok(buffer, "\0");
                while (token != NULL){
                    printf("%s\n", token);
                    token = strtok(NULL, "\0");
                }
                // printf("%s\n", buffer);
                // printf("%ld\n", strlen(buffer));
                memset(buffer, '\0', MAX_BUFF);
                // char newBuff[MAX_BUFF] = {0};
                recv(sock, buffer, MAX_BUFF, 0);
                // strcpy(buffer, "Chunk Received");
                // int n = send(sock, "Chunk Received", strlen("Chunk received"), 0);
                // int n = send(sock, buffer, strlen(buffer), 0);
                // printf("Bytes: %d\n", n);
                // strcpy(buffer, newBuff);
            }
            printf("End of list of content\n");

            continue;
        }
        else if (strcmp(command, "get") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", remote_file, local_file);
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "get ");
            strcat(buffer, remote_file);
            strcat(buffer, " ");
            strcat(buffer, local_file);

            int fd;
            if((fd = open(local_file, O_WRONLY | O_CREAT | O_TRUNC,0644))==-1){
                printf("Cannot wite local file\n");
                continue;
            }
            printf("Sending the entered file names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the file names due to some error\n");
                continue;
            }
            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            int status = handleResponseCode(buffer);
            if(status==-1){
                printf("The remote file %s doesn't exist on serevr side \n", remote_file);
                continue;
            };
            int len,lendata=0;
            char flageof;
            do{
                len = recv(sock, buffer, MAX_BUFF, 0);
                buffer[len] = '\0';
                printf("%s\n", buffer);
                if(buffer[0]=='M' || buffer[0]=='L'){
                    sscanf(buffer,"%c %d",&flageof,&lendata);
                    char temp[MAX_BUFF];
                    write(fd,buffer+3,len-3);
                    lendata -= (len-3);
                }
                else{
                    write(fd,buffer,len);
                    lendata -= len;
                }
                if(lendata<=0)send(sock, "Ack\0", strlen("Ack\0"), 0);
            }
            while(flageof!='L');
            printf("File fetched successfully\n");
            continue;
        }
        else if (strcmp(command, "put") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", local_file, remote_file);
	        char buffer[MAX_BUFF];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "put ");
            strcat(buffer, local_file);
            strcat(buffer, " ");
            strcat(buffer, remote_file);

            put(buffer,local_file,remote_file,sock);
            continue;
        }
        else if (strcmp(command, "mget") == 0){
            printf("Executing the command mget\n");
        }
        else if (strcmp(command, "mput") == 0){
            printf("Executing the command mput\n");
            char temp[MAX_BUFF];
            scanf("%s",temp);
            char *token = strtok(temp, " ");
            while(token!=NULL){
                memset(buffer, '\0', MAX_BUFF);
                strcpy(buffer, "put ");
                strcat(buffer, token);
                strcpy(buffer, " ");
                strcat(buffer, token);

                if(put(buffer,token,token,sock)==-1) break;
                token = strtok(NULL, " ");
            }

        }
        else{
            printf("No such command found\n");
        }
    }
	// if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	// {
	// 	printf("\n Socket creation error \n");
	// 	return -1;
	// }

	// serv_addr.sin_family = AF_INET;
	// serv_addr.sin_port = htons(PORT);
	
	// // Convert IPv4 and IPv6 addresses from text to binary form
	// if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	// {
	// 	printf("\nInvalid address/ Address not supported \n");
	// 	return -1;
	// }

	// if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	// {
	// 	printf("\nConnection Failed \n");
	// 	return -1;
	// }
	// send(sock , hello , strlen(hello) , 0 );
	// printf("Hello message sent\n");
	// valread = read( sock , buffer, 1024);
	// printf("%s\n",buffer );
	return 0;
}