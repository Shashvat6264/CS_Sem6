#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#define MAX_QUEUE_SIZE 8
#define MAX_JOB_ID 100000
#define N 1000

typedef struct job{
	int matrix[N][N];
	int proNo;
	int matId;
	int status;
	//0: D000 will be computed, 1: D001 , 2: D010, 3:D011 ...and so on
	//8: computation complete 
}job;

//structure to represent shared memory
typedef struct shared_memory{
	job job_queue[MAX_QUEUE_SIZE];
	int size;
	int job_created;
	int tot_matrix;
	//int tot_jobs; //no to worker process required = (tot_matrix-1)*8
	int computed;
	sem_t mutex;
	sem_t full;
	sem_t empty;
}shared_memory;

job create_job(int proNo){
	job x;
	x.proNo = proNo;
	x.matId = rand()%MAX_JOB_ID+1;
	x.status = 0;
	for (int i = 0; i < N; ++i){
		for (int j = 0; j < N; ++j){
			x.matrix[i][j] = rand()%19 - 9;
		}
	}
	return x;
}

void insert_job(shared_memory* shm,job x){
	if(shm->size==MAX_QUEUE_SIZE){
		printf("Overflow: Cannoot insert\n");
		return;
	}
	shm->job_queue[shm->size] = x;
	shm->size++;
}

void remove_job(shared_memory* shm){
	if(shm->size<=1){
		printf("Not enough job to remove\n");
		return;
	}
	shm->size-=2;
	for (int i = 0; i < shm->size; ++i){
		shm->job_queue[i] = shm->job_queue[i+2];
	}
}

void print_job(job x){
	printf("Job ID: %d\n",x.matId);
	printf("producer: %d\n",x.proNo);
	for (int i = 0; i < N; ++i){
		for (int j = 0; j < N; ++j){
			printf("%d  ",x.matrix[i][j]);
		}
		printf("\n");
	}
}


void producer(int shmid,int proNo){
	shared_memory* shmp=(shared_memory*)shmat(shmid,NULL,0);
	while(1){
		// if all jobs created, exit
		if(shmp->job_created==shmp->tot_matrix)
			break;
		// create job
		job j = create_job(proNo);
		// random delay
		sleep(rand()%4);
		// wait for empty semaphore
		sem_wait(&(shmp->empty));
		// wait for mutex
		sem_wait(&(shmp->mutex));
		// if all jobs created, exit
		if(shmp->job_created==shmp->tot_matrix){
			sem_post(&(shmp->mutex));
			break;
		}
		if(shmp->size < MAX_QUEUE_SIZE){
			insert_job(shmp,j);
			printf("Produced job details:\n");
			print_job(j);
			// increment shared variable
			shmp->job_created++;
			// signal the full semaphore		
			sem_post(&(shmp->full));						
		}
		// signal mutex
		sem_post(&(shmp->mutex));
	}
	// detach this process from shared memory
	shmdt(shmp);
}
// worker function
void worker(int shmid,int cons_no){
	shared_memory* shmc=(shared_memory*)shmat(shmid,NULL,0);
	while(1){
		// random delay
		sleep(rand()%4);
		// wait for the full semaphore
		sem_wait(&(shmc->full));
		// wait to acquire mutex
		sem_wait(&(shmc->mutex));
		// job j;
		// flag to indicate a job is retrieved
		int job_retrieved=0;
		if(shmc->size>1){
			//compute cij acc to status
			job_retrieved=1;
		}
		// signal mutex
		sem_post(&(shmc->mutex));
		if(job_retrieved){
			//print and status++ for both job
			// shmc->job_queue[0].status++;
			// shmc->job_queue[1].status++;
			// printf("Consumed job details\n");
			// printf("worker: %d,",cons_no);
			// print_job(j);
			// wait for mutex
			sem_wait(&(shmc->mutex));
			shmc->job_queue[0].status++;
			shmc->job_queue[1].status++;
			printf("Consumed job details\n");
			printf("worker: %d,",cons_no);
			// increment shared variable
			//shmc->job_completed++;
			// signal mutex
			sem_post(&(shmc->mutex));
			// signal empty semaphore
			sem_post(&(shmc->empty));
			// to ensure worker is killed only after it has slept/computed job
			sem_wait(&(shmc->mutex));
			shmc->computed++;
			sem_post(&(shmc->mutex));
		};
	}
	// detach from shared memory
	shmdt(shmc);
}


int main(){

	srand(time(0));
	//create SHM
	// key_t key = ftok("/dev/random",'c');
	// if (key<0){
	// 	printf("Error in generating key! try again..\n");
	// 	exit(1);
	// }
	int shmid = shmget(IPC_PRIVATE,sizeof(shared_memory),0666|IPC_CREAT);
	if(shmid<0){
		printf("Error in creating SHM! try again..\n");
		exit(1);
	}

	int NP,NW,tot_matrix;	
	printf("No of Producers:\n"); scanf("%d",&NP);
	printf("No workers:\n"); scanf("%d",&NW);
	printf("No of matrices to multiply:\n"); scanf("%d",&tot_matrix);
	//initialize values in SHM
	shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);
	shm->size = 0;
	shm->tot_matrix = tot_matrix;
	shm->job_created = 0;
	int tot_jobs = (tot_matrix-1)*8;
	shm->computed = 0;
	
	
	// initialize the semaphore mutex
	//binary semaphore for access to jobs_created, jobs_completed, insertion & retrieval of jobs
	int sema = sem_init(&(shm->mutex),1,1);
	//counting semaphore to check if the job_queue is full
	int full_sema = sem_init(&(shm->full),1,0);
	//counting semaphore to check if the job_queue is empty
	int empty_sema= sem_init(&(shm->empty),1,MAX_QUEUE_SIZE);
	if(sema<0||full_sema<0||empty_sema<0){
		printf("Error in initializing semaphore. Exitting..\n");
		exit(1);
	}

	time_t start = time(0);
	pid_t pid;
	//producer
	pid_t pps[NP];
	for(int i=1;i<=NP;i++){
		pid=fork();
		if(pid<0){
			printf("Error in creating producer process. Exitting..\n");
			exit(1);
		}
		else if(pid==0){
			srand(time(0)+i);
			producer(shmid,i);
			return 0;
		}
		else{
			pps[i-1]=pid;
		}
	}
	//worker
	pid_t wps[NW];
	for(int i=1;i<=NW;i++){
		pid=fork();
		if(pid<0){
			printf("Error in creating worker process. Exitting..\n");
			exit(1);
		}
		else if(pid==0){
			srand(time(0)+NP+i);
			worker(shmid,i);
			return 0;
		}
		else{
			wps[i-1]=pid;
		}
	}

	// loop till all jobs are created and consumed
	while(1){
		// acquire lock so that while checking, state change not possible
		sem_wait(&(shm->mutex));
		if(shm->job_queue[0].status==8 && shm->job_queue[1].status==8){
			remove_job(shm);
		}
		// shm->computed ensures that worker gets killed only after it has computed/slept
		if(shm->job_created>=tot_matrix && shm->computed>=tot_jobs && shm->size==1){
			time_t end = time(0);
			int time_taken = end-start;
			printf("Time taken to run %d jobs= %d seconds\n",tot_matrix,time_taken);
			// kill all child processes
			for(int i=0;i<NP;i++)
				kill(pps[i],SIGTERM);
			for(int i=0;i<NW;i++)
				kill(wps[i],SIGTERM);
			sem_post(&(shm->mutex));
			break;		
		}
		sem_post(&(shm->mutex));
	}
	//destroy mutex semaphore
	sem_destroy(&(shm->mutex));
	//sem_destroy(&(shm->full));
	//sem_destroy(&(shm->empty));
	shmdt(shm);//detach shared memory segment
	shmctl(shmid,IPC_RMID,0);//mark shared memory segment to be destroyed
	return 0;
}

