#include "rsocket.h"


int dropMessage(float prob){
	double t = (double)rand() / (double)RAND_MAX ;
	return t<prob;
}


void *thread_R(){
	while(!bind_flag);
	while(1){
		//sleep(T);
		char temp[100];
		int n;
		struct sockaddr_in cliaddr;
	    memset(&cliaddr, 0, sizeof(cliaddr));
		socklen_t len = sizeof(cliaddr);
		
		// printf("%d\n",sockfd);
		n = recvfrom(sockfd, temp , 100 , 0, ( struct sockaddr *) &cliaddr,&len);
		if(n<0){
			perror("Error in recieving message\n");
			continue;
		}
		if (dropMessage(p))
			continue;


		if(temp[0]=='A' && temp[1]=='C' && temp[2]=='K' ){
			int seq_no = 0;
			seq_no += temp[3]-'0';
			seq_no *= 10;
			seq_no += temp[4]-'0';


			utable_len--;
			// utable_next = seq_no;
			pthread_mutex_lock(&utable_lock);
			memset(unack_table[seq_no].buf,'\0', MAX );
			pthread_mutex_unlock(&utable_lock);
		}
		else if(strlen(temp)>0){
			int seq_no = 0;
			seq_no += temp[0]-'0';
			seq_no *= 10;
			seq_no += temp[1]-'0';


			pthread_mutex_lock(&rtable_lock);
			int i;
			for (i = 0; i < MAXTABLESIZE; ++i){
				//find empty place in table
				if(strlen(recv_table[i].buf)==0){
					// rtable_next = i+1;
					break;
				}
			}
			strcpy(recv_table[i].buf, temp);
			recv_table[i].src_addr = (struct sockaddr*)&cliaddr;
			rtable_len++;
 			pthread_mutex_unlock(&rtable_lock);

 			//send ack 
 			char ack_msg[6];
 			ack_msg[0] = 'A';
 			ack_msg[1] = 'C';
 			ack_msg[2] = 'K';
 			ack_msg[3] = temp[0];
 			ack_msg[4] = temp[1];
 			ack_msg[5] = '\0';

 			sendto(sockfd,ack_msg,strlen(ack_msg),0,(const struct sockaddr*)&cliaddr,len);
		}

	}

}
void *thread_S(){

	while(!bind_flag);

	while(1){
		sleep(T);
		pthread_mutex_lock(&utable_lock);
		for (int i = 0; i < MAXTABLESIZE; ++i){
			if(strlen(unack_table[i].buf)>0 && (time(0)-unack_table[i].sent_time>=2*T)){
				char temp[MAX];
				strcpy(temp,unack_table[i].buf);
				struct sockaddr *daddr = unack_table[i].dest_addr;
				socklen_t daddr_len = sizeof(*daddr);
				sendto(sockfd,temp,strlen(temp),0,(struct sockaddr *) daddr,daddr_len);
				no_of_tranmission++;
				unack_table[i].sent_time = time(0);
			}
		}
		pthread_mutex_unlock(&utable_lock);
	}
}

int r_socket(int domain, int type, int protocol){

    sockfd = socket(domain, type, protocol);
    
    //initialize tables
    unack_table = (send_msg*)malloc(MAXTABLESIZE*sizeof(send_msg));
    recv_table = (recv_msg*)malloc(MAXTABLESIZE*sizeof(recv_msg));
    utable_len = 0;
    rtable_len = 0;
    tzero = time(0);
    bind_flag = 0;
    no_of_tranmission = 0;

    
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
	bind_flag = 1;
    return bind(sockfd, addr, addrlen);
}

int r_sendto(int sockfd, const void *buf, size_t len, struct sockaddr *dest_addr, socklen_t addrlen){
	
	// while(utable_len>=MAXTABLESIZE) 
		// sleep(T);

	pthread_mutex_lock(&utable_lock);
	int i;
	for (i = 0; i < MAXTABLESIZE; ++i){
		//find empty place in table
		if(strlen(unack_table[i].buf)==0){
			// utable_next = i+1;
			break;
		}
	}
	char *buf2 = strdup((char*)buf);
	char temp[len+3];
	temp[0] = '0'+(i/10);
	temp[1] = '0'+(i%10);

	for(int j=0;j<len;j++){
		temp[j+2] = buf2[j];
	}
	temp[len+2] = '\0';
	utable_len++;
	strcpy(unack_table[i].buf,temp);

	unack_table[i].dest_addr = dest_addr;
	int n = sendto(sockfd, temp, len+3 ,0, (const struct sockaddr *) dest_addr,addrlen);
	no_of_tranmission++;
	unack_table[i].sent_time = time(0);
	pthread_mutex_unlock(&utable_lock);

	return n;
}

int r_recvfrom(int sockfd,void *buf, size_t len, const struct sockaddr *src_addr, socklen_t addrlen){
	//return recvfrom(sockfd, buf, len ,0, ( struct sockaddr *) src_addr,&addrlen);
	while(rtable_len==0) 
		sleep(1);

	pthread_mutex_lock(&rtable_lock);
	int i;
	for (i = 0; i < MAXTABLESIZE; ++i){
		if(strlen(recv_table[i].buf)>0){
			// rtable_next = i;
			break;
		}
	}
	strcpy(buf, recv_table[i].buf);
	len = strlen(recv_table[i].buf);
	memset(recv_table[i].buf,'\0',MAX);
	rtable_len--;
	pthread_mutex_unlock(&rtable_lock);
	return len;
}

int r_close(int fd){
        
    // kill threads
    pthread_kill(R, 1);
    pthread_kill(S, 1);

    // free memory
    free(unack_table);
    free(recv_table);

    return close(fd);
}
