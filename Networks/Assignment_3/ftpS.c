#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUFF 100
#define PORT 3000
#define BLK_SIZE 100

#define MAXUSERS 100


int getAllUsers(char **usernames, char **passwords){
    FILE *fp = fopen("user.txt", "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

	int i = 0;
	while((read = getline(&line, &len, fp)) != -1){
		char *newline = (char *)malloc(sizeof(char)*MAX_BUFF);
		strcpy(newline, line);
		char *token = strtok(newline, " ");
		usernames[i] = token;
		token = strtok(NULL, " ");
		if (token[strlen(token)-1] == '\n') token[strlen(token)-1] = '\0';
		passwords[i] = token;
		i++;
	}

	fclose(fp);
    return i;
}

int checkUserName(int noOfUsers, char **usernames, char *username){
	for (int i=0;i<noOfUsers;i++){
		if (strcmp(username, usernames[i]) == 0) return 1;
	}
	return 0;
}

int checkPassWord(int noOfUsers, char **passwords, char **usernames, char *password, char *username){
	for (int i=0;i<noOfUsers;i++){
		if (strcmp(username, usernames[i]) == 0){
			if (strcmp(password, passwords[i]) == 0) return 1;
		}
	}
	return 0;
}

int sendDefaultStatusResponse(int sock, int status){
	if (status){
		return send(sock, "200", strlen("200"), 0) < 0;
	}
	else{
		return send(sock, "500", strlen("500"), 0) < 0;
	}
}

int main(int argc, char *argv[]){
	int sockfd, newsockfd; 
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	int opt = 1;

	int i;
	char buffer[MAX_BUFF];

	char **usernames, **passwords;
	usernames = (char **)malloc(sizeof(char *)*MAXUSERS);
	passwords = (char **)malloc(sizeof(char *)*MAXUSERS);

    int noOfUsers = getAllUsers(usernames, passwords);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	if (listen(sockfd, 3) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

    printf("Server running on %s on port %d..\n", inet_ntoa(serv_addr.sin_addr), htons(serv_addr.sin_port));

	while (1) {
		clilen = sizeof(cli_addr);
		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0){
			perror("Error in connecting to client.\n");
		}
		else{
			printf("Accepted a client connection.\n");

			if (newsockfd < 0) {
				printf("Accept error\n");
				exit(0);
			}

			if (fork() == 0) {
				close(sockfd);
				char username[MAX_BUFF], password[MAX_BUFF];
				int username_verified = 0, password_verified = 0;

				while(1){
					memset(buffer, '\0', MAX_BUFF);
					recv(newsockfd, buffer, 100, 0);

					char *token = strtok(buffer, " ");

					if (strcmp(token, "user") == 0){
						char *token = strtok(NULL, " ");
						strcpy(username, token);
						printf("Received user command to log in %s from client\n", username);
						int status = checkUserName(noOfUsers, usernames, username);
						if (status){
							printf("Found the username\n");
							username_verified = 1;
						} 
						else{
							printf("No such username exists\n");
							username_verified = 0;
						} 	
						if (sendDefaultStatusResponse(newsockfd, status) < 0){
							printf("Could Not send response due to some error\n");
						}
						continue;
					}
					else if (!username_verified){
						printf("Username not verified yet\n");
						send(newsockfd, "600", strlen("600"), 0);
						continue;
					}
					else if (strcmp(token, "pass") == 0){
						char *token = strtok(NULL, " ");
						strcpy(password, token);
						printf("Received password %s for username %s\n", password, username);
						int status = checkPassWord(noOfUsers, passwords, usernames, password, username);
						if (status){
							printf("Found the user for the given username and password\n");
							password_verified = 1;
						}
						else{
							printf("Given username does not have this password\n");
							username_verified = 0;
							password_verified = 0;
						}
						if (sendDefaultStatusResponse(newsockfd, status) < 0){
							printf("Could Not send response due to some error\n");
						}
						continue;
					}
					else if (!password_verified){
						printf("Password not verified yet\n");
						send(newsockfd, "600", strlen("600"), 0);
						continue;
					}
					else if (strcmp(token, "cd") == 0){
						char *dir_name;
						dir_name = strtok(NULL, " ");
						printf("Received cd command to change to directory %s\n", dir_name);
						int status = chdir(dir_name) + 1;
						if (status){
							printf("Shifted server to directory %s\n", dir_name);
						}
						else{
							printf("Could not find the path %s\n", dir_name);
						}
						if (sendDefaultStatusResponse(newsockfd, status) < 0){
							printf("Could Not send response due to some error\n");
						}
						continue;
					}
					else if (strcmp(token, "dir") == 0){
						printf("Received dir command\n");
					}
					else if (strcmp(token, "get") == 0){
						char *remote_file, *local_file;
						remote_file = strtok(NULL, " ");
						local_file = strtok(NULL, " ");
						printf("Received get command for local directory: %s and remote directory: %s\n", local_file, remote_file);
						int fd;
						int status = 1;
						if((fd=open(remote_file,O_RDONLY))==-1){
							printf("The requested file does not exist\n");
							status = 0;
						}
						if (sendDefaultStatusResponse(newsockfd, status) < 0){
							printf("Could Not send response due to some error\n");
						}
						if(status){
							
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
				                    if(send(newsockfd, temp, strlen(temp), 0) <0 ){
				                        printf("Could Not send response due to some error\n");
				                        break;
				                    }
				                    break;
				                }
				                if(send(newsockfd, temp, strlen(temp), 0) <0 ){
				                    printf("Could Not send response due to some error\n");
				                    break;
				                }

				                if(send(newsockfd, buffile, len, 0) <0 ){
				                    printf("Could Not send response due to some error\n");
				                    break;
				                }
				            }
				            printf("file over!\n");
				            close(fd);
				            

						}
						continue;
					}
					else if (strcmp(token, "put") == 0){
						char *remote_file, *local_file;
						local_file = strtok(NULL, " ");
						remote_file = strtok(NULL, " ");
						printf("Received put command for local file: %s and remote file: %s\n", local_file, remote_file);
						int status = 1;
						int fd;
						if((fd=open(remote_file,O_WRONLY | O_CREAT | O_TRUNC,0644))==-1){
							printf("The requested file does not exist\n");
							status = 0;
						}
						if (sendDefaultStatusResponse(newsockfd, status) < 0){
							printf("Could Not send response due to some error\n");
						}
						if(status){
							int len,lendata=0;
				            char flageof;
				            while(1){
				                if((len = recv(newsockfd, buffer, 3, MSG_WAITALL))<0){
									printf("Could Not receive message due to some error\n");
				                }
				                buffer[len] = '\0';
				                sscanf(buffer,"%c %d",&flageof,&lendata);
				                if(flageof=='L') break;
				                if((len = recv(newsockfd, buffer, lendata,MSG_WAITALL))<0){
									printf("Could Not receive message due to some error\n");
				                }
		                		buffer[len] = '\0';
				                write(fd,buffer,len);
			                	/*while(lendata>0){
			                		if((len = recv(newsockfd, buffer, min(lendata,BLK_SIZE),0))<0){
										printf("Could Not receive message due to some error\n");
					                }
			                		buffer[len] = '\0';
			                		printf("data:   %s\n", buffer);
					                write(fd,buffer,len);
				                    lendata -= len;
				                
				                }*/
				                //if(lendata<=0)send(newsockfd, "Ack\0", strlen("Ack\0"), 0);
				            }
				            printf("File fetched successfully\n");
						}
						continue;
					}
				}

				close(newsockfd);
				exit(0);
			}
			close(newsockfd);
		}
	}
}
