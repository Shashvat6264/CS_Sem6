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


int min(int a,int b){
    if(a>b) return b;
    return a;
}

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
                printf("Cannot read local file\n");
                return -1;
            }
            printf("Sending the entered directory names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory names due to some error\n");
                return -1;
            }
            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, MAX_BUFF, 0);
            //send(sock,"Ack\0",strlen("Ack\0"),0);
            int status = handleResponseCode(buffer);
            if(status==-1){
                printf("The remote file %s doesn't exist on serevr side \n", remote_file);
                return -1;
            };
            
            while(1){   
                char buffile[BLK_SIZE];
                char temp[4];
                memset(temp, '\0', 4);
                memset(buffile,'\0',BLK_SIZE);
                short int len = read(fd, buffile, 20);
                sprintf(temp,"M%2d",len); 
                //printf("%d %s\n",len,buffile );
                if(len==0){
                    temp[0] = 'L';
                    if(send(sock, temp, strlen(temp), 0) <0 ){
                        printf("Could Not send response due to some error\n");
                        break;
                    }
                    break;
                }
                printf("headers:   %s\n", temp);
                if(send(sock, temp, strlen(temp), 0) <0 ){
                    printf("Could Not send response due to some error\n");
                    break;
                }

                printf("data:   %s\n", buffile);
                if(send(sock, buffile, len, 0) <0 ){
                    printf("Could Not send response due to some error\n");
                    break;
                }
            }
            printf("file over!\n");
            close(fd);
            return 1;
}

int get(char*buffer,char *local_file, char *remote_file, int sock){
            int fd;
            if((fd = open(local_file, O_WRONLY | O_CREAT | O_TRUNC,0644))==-1){
                printf("Cannot write local file\n");
                return -1;
            }
            printf("Sending the entered directory names to server\n");
            if (send(sock, buffer, strlen(buffer), 0) < 0){
                printf("Could not send the directory names due to some error\n");
                return -1;
            }
            memset(buffer, '\0', MAX_BUFF);
            recv(sock, buffer, 3, MSG_WAITALL);
            //send(sock,"Ack\0",strlen("Ack\0"),0);
            int status = handleResponseCode(buffer);
            if(status==-1){
                printf("The remote file %s doesn't exist on serevr side \n", remote_file);
                return -1;
            };
            int len,lendata=0;
            char flageof;
            while(1){
                if((len = recv(sock, buffer, 3, MSG_WAITALL))<0){
                    printf("Could Not receive message due to some error\n");
                }
                buffer[len] = '\0';
                sscanf(buffer,"%c %d",&flageof,&lendata);
                if(flageof == 'L') break;
                if((len = recv(sock, buffer, lendata,MSG_WAITALL))<0){
                    printf("Could Not receive message due to some error\n");
                }
                buffer[len] = '\0';
                write(fd,buffer,len);
                /*while(lendata>0){
                    if((len = recv(sock, buffer, min(lendata,BLK_SIZE),0))<0){
                        printf("Could Not receive message due to some error\n");
                    }
                    buffer[len] = '\0';
                    printf("data:   %s\n", buffer);
                    write(fd,buffer,len);
                    lendata -= len;
                
                }*/
                //if(lendata<=0)send(sock, "Ack\0", strlen("Ack\0"), 0);
            }

            printf("File fetched successfully\n");
            return 1;
}



int main(int argc, char const *argv[]){
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
            char buffer[1024];
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
            char buffer[1024];
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
            char buffer[1024];
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
            int break_loop = 0;
            while (!break_loop){
                memset(buffer, '\0', MAX_BUFF);
                int n = recv(sock, buffer, MAX_BUFF, 0);
                if (n<0){
                    printf("Error in receiving data\n");
                    return 0;
                }
                else if (n == 0) break_loop = 1;
                else{
                    char last = '\0';
                    for (int i=0;i<n && !break_loop;i++){
                        if (buffer[i] == '\0'){
                            if (last == '\0') break_loop = 1;
                            else printf("\n");
                        }
                        else printf("%c", buffer[i]);
                        last = buffer[i];
                    }
                }
            }
            printf("End of list of content\n");

            continue;
        }
        else if (strcmp(command, "get") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", remote_file, local_file);
            char buffer[1024];
            memset(buffer, '\0', MAX_BUFF);
            strcpy(buffer, "get ");
            strcat(buffer, remote_file);
            strcat(buffer, " ");
            strcat(buffer, local_file);

            get(buffer,local_file,remote_file,sock);
            continue;
        }
        else if (strcmp(command, "put") == 0){
            char remote_file[MAX_BUFF], local_file[MAX_BUFF];
            scanf("%s %s", local_file, remote_file);
            char buffer[1024];
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
            char temp[MAX_BUFF];
            scanf("%[^\n]",temp);
            char buffer[1024];
            char *token = strtok(temp, " ");
            while(token!=NULL){
                memset(buffer, '\0', MAX_BUFF);
                char remote_file[MAX_BUFF], local_file[MAX_BUFF];
                strcpy(local_file,token);
                strcpy(remote_file,token);
                strcpy(buffer, "get ");
                strcat(buffer, local_file);   
                strcat(buffer, " ");
                strcat(buffer, remote_file);

                if(get(buffer,local_file,remote_file,sock)==-1) break;
                token = strtok(NULL, " ");
            }
        }
        else if (strcmp(command, "mput") == 0){
            printf("Executing the command mput\n");
            char temp[MAX_BUFF];
            scanf("%[^\n]",temp);
            char buffer[1024];
            char *token = strtok(temp, " ");
            while(token!=NULL){
                memset(buffer, '\0', MAX_BUFF);
                char remote_file[MAX_BUFF], local_file[MAX_BUFF];
                strcpy(local_file,token);
                strcpy(remote_file,token);
                strcpy(buffer, "put ");
                strcat(buffer, local_file);   
                strcat(buffer, " ");
                strcat(buffer, remote_file);

                if(put(buffer,local_file,remote_file,sock)==-1) break;
                token = strtok(NULL, " ");
            }

        }
        else{
            printf("No such command found\n");
        }
    }
	return 0;
}