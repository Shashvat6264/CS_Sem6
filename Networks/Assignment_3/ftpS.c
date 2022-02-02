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


int main(int argc, char *argv[]){
	int sockfd, newsockfd; 
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	int opt = 1;

	int i;
	char buffer[MAX_BUFF];

	char usernames[MAX_BUFF][MAX_BUFF];
	int user_fd = open("user.txt", O_RDONLY);

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
				char *username;

				while(1){
					memset(buffer, '\0', MAX_BUFF);
					recv(newsockfd, buffer, 100, 0);

					char *token = strtok(buffer, " ");

					if (strcmp(token, "user") == 0){
						username = strtok(NULL, " ");
						printf("Received user command to log in %s from client\n", username);
					}
					else if (strcmp(token, "pass") == 0){
						char *password;
						password = strtok(NULL, " ");
						printf("Received password %s for username %s\n", password, username);
					}
					else if (strcmp(token, "cd") == 0){
						char *dir_name;
						dir_name = strtok(NULL, " ");
						printf("Received cd command to change to directory %s\n", dir_name);
					}
					else if (strcmp(token, "dir") == 0){
						printf("Received dir command\n");
					}
					else if (strcmp(token, "get") == 0){
						char *remote_file, *local_file;
						remote_file = strtok(NULL, " ");
						local_file = strtok(NULL, " ");
						printf("Received get command for local directory: %s and remote directory: %s\n", local_file, remote_file);
					}
					else if (strcmp(token, "put") == 0){
						char *remote_file, *local_file;
						local_file = strtok(NULL, " ");
						remote_file = strtok(NULL, " ");
						printf("Received put command for local directory: %s and remote directory: %s\n", local_file, remote_file);	
					}

				}

				close(newsockfd);
				exit(0);
			}
			close(newsockfd);
		}
	}
}
			
