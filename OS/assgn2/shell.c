#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXCMDLEN 200
#define MAXCMDNUM 50
#define MAXPIPES 20


int  inputCmd(char* str){
	//str = (char*) malloc(MAXCMDLEN * sizeof(char));
	int n = scanf("%[^\n]", str);
	//printf("%d %s\n", n,str);
	return n;

}

char** parseCmd(char* cmd){
	//printf("parsingg %s\n", str);
	char *token, *str;
	char** words= (char**)malloc(sizeof(char*)*MAXCMDNUM);
	str = strdup(cmd);
	token = strtok(str, " ");
	int i=0;
    while (token != NULL){
    	words[i++] = token;
        printf("%s\n", words[i-1]);
        token = strtok(NULL, " ");
    }
    return words;
}

int parsePipes(char* cmd, char** cmdlist){

}

void initShell(){
	printf("/******************************************************************\n");
	printf("*******************************************************************\n");
	printf("************************ Welcome to our shell *********************\n");
	printf("*******************************************************************\n");
	printf("******************************************************************/\n");
}

void printDirName(){
	char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s", cwd);
}


int main()
{
	initShell();

	while(1){

		printDirName();

		char* cmd;
		char **parsedcmd;
		printf("/myshell$ ");
		cmd  = (char*) malloc(MAXCMDLEN * sizeof(char));
		 //get prev newline
		int notBlank = inputCmd(cmd);
		getchar();
		//if input is blank handle
		if(!notBlank) continue;


		parsedcmd = parseCmd(cmd);
		//printf("##%s\n", cmd);
		pid_t pid;
		pid = fork();
		if(pid<0){
		   fprintf(stderr, "fork failed");
		   return 1;    
		}
		else if(pid == 0){  
		   execlp(cmd,cmd,(char*)NULL);}
		else{
		   wait(NULL);
		   printf("child complete\n");
		}

	}
	return 0;
}