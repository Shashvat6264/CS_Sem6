#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define T 2
#define p 0.5
#define SOCK_MRP SOCK_DGRAM
#define MAX 100

pthread_t R,S;
pthread_mutex_t ulock,rlock;

typedef struct _send_msg{
	char buf[MAX];
	time_t sent_time;
	const struct sockaddr *dest_addr;
} send_msg;

typedef struct _unack_table{
	send_msg utable[50];
	int entries;
}unack_table;


int r_socket(int domain, int type, int protocol)

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int r_sendto(int sockfd, const void *buf, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen);

int r_recvfrom(int sockfd,void *buf, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen);

int r_close(int fd);