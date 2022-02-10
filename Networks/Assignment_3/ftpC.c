#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8080
#define MAX_BUFF 100

// Utility function to print message according to response
int handleResponseCode(char *response_status){
    if (strcmp(response_status, "200") == 0){
        printf("Command executed successfully\n");
        return 1;
    }
    else if (strcmp(response_status, "500") == 0){
        printf("Error executing command\n");
        return -1;
    }
    else if (strcmp(response_status, "600") == 0){
        printf("Incorrect order of commands\n");
        return -2;
    }
    else return 0;
}

int main(int argc, char const *argv[])
{
	int sock = 0, valread;
	struct sockaddr_in serv_addr;

	char buffer[1024] = {0};

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
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "cd ");
            strcat(buffer, dir_name);

            printf("Sending the entered directory name to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory name due to some error\n");
                continue;
            }
        }
        else if (strcmp(command, "lcd") == 0){
            printf("Executing the command pass\n");
        }
        else if (strcmp(command, "dir") == 0){
            printf("Executing the command pass\n");
        }
        else if (strcmp(command, "get") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", remote_file, local_file);
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "get ");
            strcat(buffer, remote_file);
            strcat(buffer, " ");
            strcat(buffer, local_file);

            printf("Sending the entered directory names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory names due to some error\n");
                continue;
            }
        }
        else if (strcmp(command, "put") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", local_file, remote_file);
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "put ");
            strcat(buffer, local_file);
            strcat(buffer, " ");
            strcat(buffer, remote_file);

            printf("Sending the entered directory names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory names due to some error\n");
                continue;
            }
        }
        else if (strcmp(command, "mget") == 0){
            printf("Executing the command pass\n");
        }
        else if (strcmp(command, "mput") == 0){
            printf("Executing the command pass\n");
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