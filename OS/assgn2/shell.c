#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAXCMDLEN 200
#define MAXCMDNUM 50
#define MAXPIPES 20

#define HISTFILESIZE 10000
#define HISTSHOWSIZE 1000

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

void sigint_handler(int signum){
	signal(SIGINT, sigint_handler);
	printf("Got a Ctrl+C interrupt\n");
	pid_t pid = getpid();
	printf("Ending process with PID: %d", pid);
	kill(pid, SIGKILL);
	fflush(stdout);
}

void add_history(char *command){

}

int inputCmd(char* str){
	//str = (char*) malloc(MAXCMDLEN * sizeof(char));
	int n = scanf("%[^\n]", str);
	add_history(str);
	getchar();
	//printf("%d %s\n", n,str);
	return n;
}

char** parseCmd(char* cmd, char** words){
	//printf("parsingg %s\n", str);
	char* token, *str;
	// char **words= (char **)malloc(sizeof(char*)*MAXCMDNUM);
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

void openHelp()
{
    puts("\n***WELCOME TO MY SHELL HELP***"
        "\nCopyright @ Suprotik Dey"
        "\n-Use the shell at your own risk..."
        "\nList of Commands supported:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>all other general commands available in UNIX shell"
        "\n>pipe handling"
        "\n>improper space handling");
  
    return;
}

int ownCommandHandler(char** parsed){
	int NoOfOwnCmds = 4, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
  
    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }
	
    switch (switchOwnArg) {
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        chdir(parsed[1]);
        return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
        username = getenv("USER");
        printf("\nHello %s.\nMind that this is "
            "not a place to play around."
            "\nUse help to know more..\n",
            username);
        return 1;
    default:
        break;
    }
  
    return 0;
}


int main()
{
	initShell();
	signal(SIGINT, sigint_handler);

	while(1){

		printDirName();

		char cmd[MAXCMDLEN];
		char* parsedcmd[MAXCMDNUM];
		printf("/myshell$ ");
		
		//if input is blank handle
		if(!inputCmd(cmd)) continue;

		parseCmd(cmd, parsedcmd);

		// if (ownCommandHandler(parseCmd)) continue;

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