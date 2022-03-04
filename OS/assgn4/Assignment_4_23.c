#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXPRODTHREADS 1000
#define MAXCONSTHREADS 1000
#define MAXCHILDJOBS 1000
#define MAXJOBS 1000

enum Status {Waiting, OnGoing, Done};
enum Log {Adding, Starting, Completed};

typedef struct node{
    unsigned long jid;
    struct node *dependantj[MAXCHILDJOBS];
    struct node *parent;
    enum Status status;
    sem_t mutex;
    int cntchildjobs;
    int waitingjobs;

    // Job Data
    float jruntime;
} Node;

typedef struct shared_memory{
    struct node nodeArr[MAXJOBS];
    Node *head;
    int noOfNodes;
} shared_memory;

// Utility functions
void print_job(Node n){
    printf("Job %lu\n", n.jid);
    if (n.status == Waiting) printf("\tStatus: Waiting\n");
    else if (n.status == OnGoing) printf("\tStatus: OnGoing\n");
    else if (n.status == Done) printf("\tStatus: Done\n");
    printf("\tRuntime: %f\n", n.jruntime);
}

void job_logs(enum Log log, Node *parent, Node *child){
    switch (log)
    {
    case Adding:
        if (parent != NULL && child != NULL){
            printf("\n---------Added new job---------\n");
            printf("Parent Job:\n");
            print_job(*parent);
            printf("\nNew job:\n");
            print_job(*child);
        }
        else{
            printf("Improper use of job logger");
        }
        break;

    case Starting:
        if (parent != NULL){
            printf("\n---------Starting new job---------\n");
            print_job(*parent);
        }
        break;
    
    case Completed:
        if (parent != NULL){
            printf("\n---------Completed a job---------\n");
            print_job(*parent);
        }
        break;
    
    default:
        break;
    }
}

int generateJID(){
    unsigned long x;
    x = rand();
    x <<= 15;
    x ^= rand();
    x %= 100000001;
    return x;
}

void run_job(Node *n){
    job_logs(Starting, n, NULL);
    n->status = OnGoing;
    sleep(n->jruntime);
    job_logs(Completed, n, NULL);
    n->status = Done;
}

Node create_job(Node *parent){
    Node n;
    n.jid = generateJID();
    n.jruntime = (rand()%251)/1000.0;
    n.parent = parent;
    n.status = Waiting;
    n.cntchildjobs = 0;
    n.waitingjobs = 0;
    int sem_job = sem_init(&(n.mutex),1,1);
    if (sem_job < 0){
        perror("Could not create semaphore\n");
        exit(1);
    }
    return n;
}

void add_job_dependancy(int shmid, Node *parent){
	shared_memory* shmc = (shared_memory*)shmat(shmid,NULL,0);
    shmc->nodeArr[shmc->noOfNodes] = create_job(parent);
    parent->dependantj[parent->cntchildjobs] = &shmc->nodeArr[shmc->noOfNodes];
    job_logs(Adding, parent, &shmc->nodeArr[shmc->noOfNodes]);
    parent->cntchildjobs++;
    parent->waitingjobs++;
    shmc->noOfNodes++;
    shmdt(shmc);
}

void *producer_thread_function(void *input){
    int shmid = *(int *)input;
    shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);
    while(1){
        add_job_dependancy(shmid, &shm->nodeArr[(rand()%(shm->noOfNodes))]);
        sleep((rand()%501)/1000.0);
    }
    shmdt(shm);
}

void *consumer_thread_function(void *input){
    int shmid = *(int *)input;
    shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);

    int finish = 0;
    while (!finish){
        Node *n = shm->head;
        while (n->waitingjobs > 0){
            int next = 0;
            for (int i=0;i<n->cntchildjobs && !next;i++){
                if (n->dependantj[i]){
                    if (n->dependantj[i]->status == Waiting){
                        n = n->dependantj[i];
                        next = 1;
                    }
                }
            }
        }
        if (n->status == Waiting){
            run_job(n);
        }
        if (n->parent){
            n->parent->waitingjobs--;
        }
    }

    shmdt(shm);
}

int main(int argc, char **argv){
    int p, y;
    pthread_t prod_thread_id[MAXPRODTHREADS];
    pthread_t cons_thread_id[MAXCONSTHREADS];

    printf("Number of producer threads: ");
    scanf("%d", &p);
    printf("Number of consumer threads: ");
    scanf("%d", &y);
    
	int shmid = shmget(IPC_PRIVATE,sizeof(shared_memory),0666|IPC_CREAT);
	if(shmid<0){
		printf("Error in creating SHM! try again..\n");
		exit(1);
	}
	shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);

    pid_t master_pid = fork();
    // Workflow of master process A
    if (master_pid == 0){

        // Creating a base tree T with 300 - 500 nodes //
        int noOfBaseTJobs = (rand()%201)+300;

        // Creating the head job
        shm->nodeArr[shm->noOfNodes] = create_job(NULL);
        shm->head = &shm->nodeArr[shm->noOfNodes];
        shm->noOfNodes++;

        // Creating other jobs
        for (int i=1;i<noOfBaseTJobs;i++){
            add_job_dependancy(shmid, &shm->nodeArr[(rand()%(shm->noOfNodes))]);
        }
        // Created a base tree T with "noOfBaseTJobs" amount of jobs //

        // Creating producer threads and process B //
        // Creating "p" producer threads 
        for (int i=0;i<p;i++){
            pthread_create(&prod_thread_id[i], NULL, producer_thread_function, (void *)&shmid);
        }

        // Creating a child process B //
        pid_t b_pid = fork();
        // Workflow for process B
        if (b_pid == 0){
            // Creating "y" consumer threads
            for (int i=0;i<y;i++){
                pthread_create(&cons_thread_id[i], NULL, consumer_thread_function, (void *)&shmid);
            }

            // Waiting for consumer threads to complete execution
            for (int i=0;i<y;i++){
                pthread_join(cons_thread_id[i], NULL);
            }
            shmdt(shm);
            exit(0);
        }
        // Created producer threads and process B //
        
        // Continued workflow for process "A"
        // Waiting for producer threads to complete execution
        for(int i=0;i<p;i++){
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += (rand()%11) + 10;
            pthread_timedjoin_np(prod_thread_id[i], NULL, &ts);
        }

	    shmdt(shm);
        exit(0);
    }
    wait(NULL);
    
    // Flow of base program
	shmdt(shm);
	shmctl(shmid,IPC_RMID,0);
}