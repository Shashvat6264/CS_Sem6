/***********************
Group Number: 23
Team Members: Shashvat Gupta - 19CS30042
              Sunanda Mondal - 19CS10060
***********************/

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

#define MAXPRODTHREADS 100000
#define MAXCONSTHREADS 100000
#define MAXCHILDJOBS 100000
#define MAXJOBS 100000


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
    printf("\tMax child count: %d\n", n.cntchildjobs);
    printf("\tWaiting child count: %d\n", n.waitingjobs);
}

void job_logs(int shmid, enum Log log, Node *parent, Node *child){
    shared_memory* shmc = (shared_memory*)shmat(shmid,NULL,0);
    switch (log){
        case Adding:
            if (parent != NULL && child != NULL){
                printf("\n---------Added new job---------\n");
                printf("Parent Job:\n");
                print_job(*parent);
                printf("\nNew job:\n");
                print_job(*child);
                printf("---------------------------------\n");
            }
            else{
                printf("Improper use of job logger");
            }
            break;

        case Starting:
            if (parent != NULL){
                printf("\n---------Starting new job---------\n");
                print_job(*parent);
                printf("---------------------------------\n");
            }
            else{
                printf("Improper use of job logger");
            }
            break;
        
        case Completed:
            if (parent != NULL){
                printf("\n---------Completed a job---------\n");
                print_job(*parent);
                printf("---------------------------------\n");
            }
            else{
                printf("Improper use of job logger");
            }
            break;
        
        default:
            break;
    }
    shmdt(shmc);
}

void print_in_console(int shmid, char *buffer){
    shared_memory *shm = (shared_memory*)shmat(shmid,NULL,0);
    pthread_mutex_lock(&(shm->print));
    printf("%s", buffer);
    pthread_mutex_unlock(&(shm->print));
    shmdt(shm);
}

int generateJID(){
    unsigned long x;
    x = rand();
    x <<= 15;
    x ^= rand();
    x %= 100000001;
    return x;
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
    int sec = (rand()%11) + 10;
    time_t endTime = time(NULL) + sec;
    while(time(NULL) < endTime){
        int id = rand()%(shm->noOfNodes);
        if (shm->nodeArr[id].status == Waiting){
            pthread_mutex_lock(&(shm->nodeArr[id].mutex));
            if (time(NULL) < endTime){
                pthread_mutex_unlock(&(shm->nodeArr[id].mutex));
                print_in_console(shmid, "Ending a producer thread... \n");
                shmdt(shm);
                exit(0);
            }
            if (shm->nodeArr[id].status == Waiting){
                pthread_mutex_lock(&(shm->mutex));
                if (time(NULL) < endTime){
                    pthread_mutex_unlock(&(shm->mutex));
                    print_in_console(shmid, "Ending a producer thread... \n");
                    shmdt(shm);
                    exit(0);
                }
                create_job(&(shm->nodeArr[shm->noOfNodes]), shm->nodeArr + id);
                print_job(shm->nodeArr[shm->noOfNodes]);
                shm->nodeArr[id].dependantj[shm->nodeArr[id].cntchildjobs] = &shm->nodeArr[shm->noOfNodes];
                shm->nodeArr[id].cntchildjobs++;
                shm->nodeArr[id].waitingjobs++;
                shm->noOfNodes++;
                pthread_mutex_unlock(&(shm->mutex));
                pthread_mutex_unlock(&(shm->nodeArr[id].mutex));
                sleep((rand()%501)/1000.0);
                pthread_mutex_lock(&(shm->print));
                job_logs(shmid, Adding, shm->nodeArr + id, &shm->nodeArr[shm->noOfNodes - 1]);
                pthread_mutex_unlock(&(shm->print));
            }
            else pthread_mutex_unlock(&(shm->nodeArr[id].mutex));
        }
    }

    print_in_console(shmid, "Ending a producer thread... \n");
    shmdt(shm);
    exit(0);
}

void *consumer_thread_function(void *input){
    int shmid = *(int *)input;
    shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);

    int finish = 0;
    while (!finish){
        Node *old_n = NULL;
        Node *n = shm->head;
        while (n->waitingjobs > 0){
            int next = 0;
            Node *mem_n = n;
            for (int i=0;i<n->cntchildjobs && !next;i++){
                if (n->dependantj[i]){
                    if (n->dependantj[i]->status == Waiting){
                        old_n = n;
                        n = n->dependantj[i];
                        next = 1;
                    }
                }
            }
            if (n == mem_n){
                n->waitingjobs = 0;
            }
        }
        if (old_n == NULL) finish = 1;
        if (n->status == Waiting){
            pthread_mutex_lock(&(n->mutex));
            if (n->waitingjobs == 0) {
                if (old_n){
                    old_n->waitingjobs--;
                }
                n->status = OnGoing;
                pthread_mutex_lock(&(shm->print));
                job_logs(shmid, Starting, n, NULL);
                pthread_mutex_unlock(&(shm->print));
                sleep(n->jruntime);
                n->status = Done;
                pthread_mutex_lock(&(shm->print));
                job_logs(shmid, Completed, n, NULL);
                pthread_mutex_unlock(&(shm->print));
            }
            pthread_mutex_unlock(&(n->mutex));
        }
    }

    print_in_console(shmid, "Ending a consumer thread... \n");

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

    // Workflow of master process A

    // Creating a base tree T with 300 - 500 nodes //
    int noOfBaseTJobs = (rand()%201)+300;

    // Creating the head job
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

    // Creating producer threads and process B //
    // Creating "p" producer threads 
    print_in_console(shmid, "Starting producer threads... \n");
    for (int i=0;i<p;i++){
        int status = pthread_create(&prod_thread_id[i], NULL, producer_thread_function, (void *)&shmid);
        if (status < 0){
            printf("Could not create producer thread... \n");
        }
    }

    // Creating a child process B //
    print_in_console(shmid, "Starting up process B... \n");
    pid_t b_pid = fork();
    // Workflow for process B
    if (b_pid == 0){
        // Creating "y" consumer threads
        print_in_console(shmid, "Starting consumer threads... \n");
        for (int i=0;i<y;i++){
            int status = pthread_create(&cons_thread_id[i], NULL, consumer_thread_function, (void *)&shmid);
            if (status < 0){
                printf("Could not create consumer thread... \n");
            }
        }

        // Waiting for consumer threads to complete execution
        for (int i=0;i<y;i++){
            pthread_join(cons_thread_id[i], NULL);
        }

        print_in_console(shmid, "Ending all consumer threads... \n");

        shmdt(shm);
        exit(0);
    }
    // Created producer threads and process B //

    waitpid(b_pid, NULL, 0);
    
    // Continued workflow for process "A"
    // Waiting for producer threads to complete execution
    for(int i=0;i<p;i++){
        pthread_join(prod_thread_id[i], NULL);
    }

    print_in_console(shmid, "Ending all producer threads... \n");
    
    // Flow of base program
	shmdt(shm);
	shmctl(shmid,IPC_RMID,0);
    exit(0);
}