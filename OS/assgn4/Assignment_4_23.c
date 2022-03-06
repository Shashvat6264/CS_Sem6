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

#define MAXPRODTHREADS 1000
#define MAXCONSTHREADS 1000
#define MAXCHILDJOBS 1000
#define MAXJOBS 1000


// Enumerations
enum Status {Waiting, OnGoing, Done};
enum Log {Adding, Starting, Completed};

typedef struct node{
    unsigned long jid;
    struct node **dependantj;
    struct node *parent;
    enum Status status;
    pthread_mutex_t mutex;
    int cntchildjobs;
    int waitingjobs;

    // Job Data
    float jruntime;
} Node;

typedef struct shared_memory{
    Node nodeArr[MAXJOBS];
    Node *head;
    int noOfNodes;

    // Mutex locks in shared memory
    pthread_mutex_t print;
    pthread_mutex_t mutex;
} shared_memory;

// Utility functions
void print_job(Node n){
    printf("Job %lu\n", n.jid);
    if (n.status == Waiting) printf("\tStatus: Waiting\n");
    else if (n.status == OnGoing) printf("\tStatus: OnGoing\n");
    else if (n.status == Done) printf("\tStatus: Done\n");
    printf("\tRuntime: %f\n", n.jruntime);
}

void job_logs(int shmid, enum Log log, Node *parent, Node *child){
    shared_memory* shmc = (shared_memory*)shmat(shmid,NULL,0);
    pthread_mutex_lock(&(shmc->print));
    switch (log){
    case Adding:
        if (parent != NULL && child != NULL){
            printf("\n---------Added new job---------\n");
            printf("Parent Job:\n");
            print_job(*parent);
            printf("\nNew job:\n");
            print_job(*child);
            printf("\n---------------------------------\n");
        }
        else{
            printf("Improper use of job logger");
        }
        break;

    case Starting:
        if (parent != NULL){
            printf("\n---------Starting new job---------\n");
            print_job(*parent);
            printf("\n---------------------------------\n");
        }
        else{
            printf("Improper use of job logger");
        }
        break;
    
    case Completed:
        if (parent != NULL){
            printf("\n---------Completed a job---------\n");
            print_job(*parent);
            printf("\n---------------------------------\n");
        }
        else{
            printf("Improper use of job logger");
        }
        break;
    
    default:
        break;
    }
    pthread_mutex_unlock(&(shmc->print));
    shmdt(shmc);
}

int generateJID(){
    unsigned long x;
    x = rand();
    x <<= 15;
    x ^= rand();
    x %= 100000001;
    return x;
}

void run_job(int shmid, Node *n){
    job_logs(shmid, Starting, n, NULL);
    n->status = OnGoing;
    sleep(n->jruntime);
    job_logs(shmid, Completed, n, NULL);
    n->status = Done;
    if (n->parent){
        n->parent->waitingjobs--;
    }
}

void create_job(Node *n, Node *parent){
    n->jid = generateJID();
    n->jruntime = (rand()%251)/1000.0;
    n->dependantj = (Node **)malloc(MAXCHILDJOBS*sizeof(Node *));
    for (int i=0;i<MAXCHILDJOBS;i++){
        n->dependantj[i] = NULL;
    }
    n->parent = parent;
    n->status = Waiting;
    n->cntchildjobs = 0;
    n->waitingjobs = 0;
    int sem_job = pthread_mutex_init(&(n->mutex),NULL);
    if (sem_job < 0){
        perror("Could not create semaphore\n");
        exit(1);
    }
}

void *producer_thread_function(void *input){
    int shmid = *(int *)input;
    shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);
    while(1){
        int id = rand()%(shm->noOfNodes);
        while (shm->nodeArr[id].status != Waiting){
            id = rand()%(shm->noOfNodes);
        }
        pthread_mutex_lock(&(shm->nodeArr[id].mutex));
        if (shm->nodeArr[id].status == Waiting){
            Node *parent = shm->nodeArr + id;
            pthread_mutex_lock(&(shm->mutex));
            create_job(&(shm->nodeArr[shm->noOfNodes]), parent);
            parent->dependantj[parent->cntchildjobs] = &shm->nodeArr[shm->noOfNodes];
            shm->noOfNodes++;
            job_logs(shmid, Adding, parent, parent->dependantj[parent->cntchildjobs]);
            parent->cntchildjobs++;
            parent->waitingjobs++;
            pthread_mutex_unlock(&(shm->mutex));
            sleep((rand()%501)/1000.0);
        }
        pthread_mutex_unlock(&(shm->nodeArr[id].mutex));
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
        if (n->parent == NULL) finish = 1;
        if (n->status == Waiting){
            pthread_mutex_lock(&(n->mutex));
            if (n->waitingjobs == 0) run_job(shmid, n);
            pthread_mutex_unlock(&(n->mutex));
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

    // Initialise shared mutex locks in shm
    pthread_mutex_init(&(shm->print), NULL);
    pthread_mutex_init(&(shm->mutex), NULL);

    pid_t master_pid = fork();
    // Workflow of master process A
    if (master_pid == 0){

        // Creating a base tree T with 300 - 500 nodes //
        int noOfBaseTJobs = (rand()%201)+300;

        // Creating the head job
        // shm->nodeArr[shm->noOfNodes] = create_job(NULL);
        create_job(&(shm->nodeArr[shm->noOfNodes]), NULL);
        shm->head = &shm->nodeArr[shm->noOfNodes];
        shm->noOfNodes++;

        // Creating other jobs
        printf("Building base tree T... \n");
        for (int i=1;i<noOfBaseTJobs;i++){
            pthread_mutex_lock(&(shm->mutex));
            Node *parent = shm->nodeArr + (rand()%(shm->noOfNodes));
            create_job(&(shm->nodeArr[shm->noOfNodes]), parent);
            parent->dependantj[parent->cntchildjobs] = &shm->nodeArr[shm->noOfNodes];
            shm->noOfNodes++;
            job_logs(shmid, Adding, parent, parent->dependantj[parent->cntchildjobs]);
            parent->cntchildjobs++;
            parent->waitingjobs++;
            pthread_mutex_unlock(&(shm->mutex));
        }
        // Created a base tree T with "noOfBaseTJobs" amount of jobs //
        // Printing children of head

        // Creating producer threads and process B //
        // Creating "p" producer threads 
        printf("Starting producer threads... \n");
        for (int i=0;i<p;i++){
            pthread_create(&prod_thread_id[i], NULL, producer_thread_function, (void *)&shmid);
        }

        // Creating a child process B //
        printf("Starting up process B... \n");
        pid_t b_pid = fork();
        // Workflow for process B
        if (b_pid == 0){
            // Creating "y" consumer threads
            printf("Starting consumer threads... \n");
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