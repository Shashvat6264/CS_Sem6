#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <wait.h>

typedef struct _process_data {
double **A;
double **B;
double **C;
int veclen, i, j;
} ProcessData;

void *mult(void *arg){
    ProcessData* pd = arg;
    int i = pd->i;
    int j = pd->j;
    pd->C[i][j] = 0;
    for (int k = 0; k < pd->veclen; ++k){
        pd->C[i][j] = pd->C[i][j] + (pd->A[i][k]*pd->B[k][j]);
    }
}

unsigned int sizeof_dm(int row, int col, size_t sizeElement){
    size_t size = row * (sizeof(void *) + (col * sizeElement));
    //size_t size = row * (col * sizeElement);
    return size;
}

void create_index(void **m, int row, int col, size_t sizeElement){
    size_t sizeRow = col * sizeElement;
    m[0] = m+row;
    for(int i=1; i<row; i++){      
        m[i] = (m[i-1]+sizeRow);
    }
}

void print_matriz(double **matrix, int row, int col){
    printf("\n");
    for(int i=0; i<row; i++){
        for(int j=0; j<col; j++)
            printf("%.2f\t",matrix[i][j]);
        printf("\n");
    }
}

int main(int pdc, char **pdv){

    double **A, **B, **C;
    int r1,c1,r2,c2,shmId;
    printf("Enter the rows and column of Matrix A: \n");
    scanf("%d %d",&r1,&c1);

    printf("Enter the rows and column of Matrix B: \n");
    scanf("%d %d",&r2,&c2);

    size_t sizeA = sizeof_dm(r1,c1,sizeof(double));
    size_t sizeB = sizeof_dm(r2,c2,sizeof(double));
    size_t sizeC = sizeof_dm(r1,c2,sizeof(double));

    //const int sz = sizeof(double) * (r1*c1 + r2*c2 + r1*c2);
    shmId = shmget(IPC_PRIVATE, (sizeA+sizeB+sizeC), IPC_CREAT|0600);    
    A = (double**)shmat(shmId, NULL, 0); 
    B = A+(r1*c1)+r1;
    C = B+(r2*c2)+r2;
    create_index((void*)A, r1, c1, sizeof(double));
    create_index((void*)B, r2, c2, sizeof(double));
    create_index((void*)C, r1, c2, sizeof(double));

    printf("Enter matrix A:\n");
    for(int i=0; i<r1; i++){
        for(int j=0; j<c1; j++)
            scanf("%lf",&A[i][j]);
    }        
    print_matriz(A, r1, c1); 
    printf("Enter matrix B:\n");
    for(int i=0; i<r2; i++){
        for(int j=0; j<c2; j++)
            scanf("%lf",&B[i][j]);
    } 
    printf("Matrix A:\n");   
    print_matriz(A, r1, c1); 
    printf("Matrix B:\n");   
    print_matriz(B, r2, c2); 

    ProcessData pd;
    pd.A = A;
    pd.B = B;
    pd.C = C;
    pd.veclen = c1;

    for (int i = 0; i < r1; ++i){
        for (int j = 0; j < c2; ++j){
            pd.i = i;
            pd.j = j;
            pid_t pid = fork();
            if (pid < 0){
                printf("Error in forking\n");
                exit(1);
            }
            else if (pid == 0){
                //printf("called child\n");
                mult(&pd);   
                // return 0;
                exit(0);
            }            
            // else{
            //     //printf("parent\n");
            // }
        }
    }
    for (int i = 0; i < r1; ++i){
        for (int j = 0; j < c2; ++j){
           wait(NULL);
        }
    }
    printf("Resultant Matrix C:\n");   
    print_matriz(C,r1,c2);

    
    return 0;
}