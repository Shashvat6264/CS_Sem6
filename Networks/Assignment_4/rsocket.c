#include "rsocket.h"
#define MAXTABLESIZE 50


int dropMessage(float prob){
	double t = (double)rand() / (double)RAND_MAX ;
	return t<prob;
}


void *thread_R(){
	while(1){
		sleep(T);
		char* temp;
		int n;

	}

}
void *thread_S(){
	while(1){
		sleep(T);
		pthread_mutex_lock(&utable_lock);
		for (int i = 0; i < MAXTABLESIZE; ++i){
			if(strlen(unack_table[i].buf)>0 && unack_table[i].sent_time-time(0)>=2*T){
				char *temp;
				strcpy(temp,unack_table[i].buf);
				const struct sockaddr *daddr = unack_table[i].dest_addr;
				sendto(sockfd,temp,strlen(temp),0,(const struct sockaddr *) daddr,sizeof(daddr));
				unack_table[i].sent_time = time(0)-tzero;
			}
		}
		pthread_mutex_unlock(&utable_lock);
	}
}

int r_socket(int domain, int type, int protocol){
    int sockfd ;
    sockfd = socket(domain, type, protocol);
    
    //initialize tables
    unack_table = (send_msg*)malloc(MAXTABLESIZE*sizeof(send_msg));
    recv_table = (recv_msg*)malloc(MAXTABLESIZE*sizeof(recv_msg));
    utable_len = 0;
    rtable_len = 0;
    utable_next = 0;
    rtable_next = 0;
    tzero = time(0);

    
    if (pthread_mutex_init(&utable_lock, NULL) != 0) {
        printf("\nError in mutex initialization\n");
        exit(0);
    }
    if (pthread_mutex_init(&rtable_lock, NULL) != 0) {
        printf("\nError in mutex initialization\n");
        exit(0);
    }
	pthread_create(&R, NULL, thread_R, NULL);
    pthread_create(&S, NULL, thread_S, NULL);

    return sockfd;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return bind(sockfd, addr, addrlen);
}

int r_sendto(int sockfd, const void *buf, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen){
	int i;
	// while(utable_len>=MAXTABLESIZE) 
		// sleep(T);

	pthread_mutex_lock(&utable_lock);
	for (i = utable_next; i < MAXTABLESIZE; ++i){
		//find empty place in table
		if(strlen(unack_table[i].buf)==0){
			utable_next = i+1;
			break;
		}
	}
	char *buf2 = (char*)buf;
	char temp[len+3];
	temp[0] = '0'+(i/10);
	temp[1] = '0'+(i%10);

	printf("%ld\n",len );
	for(int j=0;j<len;j++){
		printf("bufff  %c\n",buf2[j] );
		temp[j+2] = buf2[j];
	}
	temp[len+2] = '\0';
	utable_len++;
	strcpy(unack_table[i].buf,temp);
	unack_table[i].dest_addr = dest_addr;
	int n = sendto(sockfd, temp, len+3 ,0, (const struct sockaddr *) dest_addr,addrlen);
	unack_table[i].sent_time = time(0)-tzero;
	pthread_mutex_unlock(&utable_lock);

	return n;
}

int r_recvfrom(int sockfd,void *buf, size_t len, const struct sockaddr *src_addr, socklen_t addrlen){
	return recvfrom(sockfd, buf, len ,MSG_WAITALL, ( struct sockaddr *) src_addr,&addrlen);
}

int r_close(int fd){
    int r = close(fd);
    
    // kill threads
    pthread_join(R, NULL);
    pthread_join(S, NULL);

    // free memory
    free(unack_table);
    free(recv_table);

    return r;
}
