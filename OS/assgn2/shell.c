#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

#define MAXCMDLEN 200
#define MAXCMDNUM 50
#define MAXPIPES 20

#define HISTFILESIZE 10000
#define HISTSHOWSIZE 1000

pid_t shell_pgid;
int shell_terminal, shell_is_interactive;
struct termios shell_tmodes;

static struct termios old, current;


void initTermios(int echo){
  tcgetattr(0, &old); 
  current = old; 
  current.c_lflag &= ~ICANON; 
  if (echo) {
      current.c_lflag |= ECHO; 
  } else {
      current.c_lflag &= ~ECHO; 
  }
  tcsetattr(0, TCSANOW, &current); 
}


void resetTermios(void){
  tcsetattr(0, TCSANOW, &old);
}


char getch_(int echo){
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}


char getch(void){
  return getch_(0);
}

void initShell(){
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive){
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) kill(- shell_pgid, SIGTTIN);

        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        shell_pgid = getpid();

        if (setpgid(shell_pgid, shell_pgid) < 0){
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);

        printf("/******************************************************************\n");
        printf("*******************************************************************\n");
        printf("************************ Welcome to our shell *********************\n");
        printf("*******************************************************************\n");
        printf("******************************************************************/\n");
    }
    else exit(0);
}

void printDirName(){
	char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s", cwd);
}

void sigint_handler(int signum){
	signal(SIGINT, sigint_handler);
	printf("Got a Ctrl+C interrupt\n");
}

void sigtstp_handler(int sig_num){
	signal(SIGTSTP, sigtstp_handler);
	printf("Got a Ctrl+Z interrupt\n");
}

void add_history(char *command){

}

int inputCmd(char* str){
	int i=0,j=0;
	char c, *temp;
	temp = (char*)malloc(sizeof(char)*50);
	
	while(1){
		c = getch();
		if(c=='\n'){
			str[i] = '\0';
			break;
		}
		str[i++] = c;
		temp[j++] = c;
		if(c==' '){
			free(temp);
			temp = (char*)malloc(sizeof(char)*50);
			j=0;
		}
		if(c=='\t'){
			//searching for file will be handled
			temp[j]=='\0';
			printf("\n%s\n",temp );// frint options here
			sleep(2);
			write(STDOUT_FILENO, "\033[F", 5);
			write(STDOUT_FILENO, "\033[K", 5); 
		}
		//implement backspace
		else if(c==127){
			printf("%c",c);
			printf("\b \b");
		}
		else{
			printf("%c", c);
		}
	}

	//int n = scanf("%[^\n]", str);
	add_history(str);
	//getchar();
	//printf("%d %s\n", n,str);
	return i;
}


char *strmbtok ( char *str, char* delim, char *openQ, char *closeQ) {

    static char *token = NULL;
    char *broken = NULL;
    char *qi = NULL;
    int flag = 0;
    int openQidx = 0;

    if ( str != NULL) {
        token = str;
        broken = str;
    }
    else {
        broken = token;
        if ( *token == '\0') {
            broken = NULL;
        }
    }

    while ( *token != '\0') {
        if ( flag) {
            if ( closeQ[openQidx] == *token) {
                flag = 0;
            }
            token++;
            continue;
        }
        if ( ( qi = strchr ( openQ, *token)) != NULL) {
            flag = 1;
            openQidx = qi - openQ;
            token++;
            continue;
        }
        if ( strchr ( delim, *token) != NULL) {
            *token = '\0';
            token++;
           	while(strchr ( delim, *token) != NULL)
           		token++;
            break;
        }
        token++;
    }
    return broken;
}

int parseCmd(char* cmd, char** words){
    char acOpen[]  = {"\"[\'{"};
    char acClose[] = {"\"]\'}"};

	//printf("parsingg %s\n", str);
	char* token, *str;
	// char **words= (char **)malloc(sizeof(char*)*MAXCMDNUM);
	str = strdup(cmd);
	token = strmbtok ( str, " ", acOpen, acClose);
	int i=0;
    while (token != NULL){
    	if(strlen(token)>0)	words[i++] = token;
        //printf("%s\n", words[i-1]);
        token = strmbtok ( NULL, " ", acOpen, acClose);
    }
    return i;
}

int parsePipes(char* cmd, char** cmdlist){
	char acOpen[]  = {"\"[\'{"};
    char acClose[] = {"\"]\'}"};

	//printf("parsingg %s\n", str);
	char* token, *str;
	// char **words= (char **)malloc(sizeof(char*)*MAXCMDNUM);
	str = strdup(cmd);
	token = strmbtok ( str, "|", acOpen, acClose);
	int i=0;
    while (token != NULL){
    	if(strlen(token)>0)	cmdlist[i++] = token;
        //printf("%s\n", words[i-1]);
        token = strmbtok ( NULL, "|", acOpen, acClose);
    }
    return i;
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

int execute(char* cmd,int in_fd,int out_fd){
	int len = strlen(cmd);
	//strip spaces at the end
	while(cmd[len-1]==' '){
		len--;
	}
	cmd[len] = '\0';
	printf("%s\n", cmd);
	char** parsedcmd;
	parsedcmd = (char**)malloc(MAXCMDNUM*sizeof(char*));
	int noWords = parseCmd(cmd, parsedcmd);
	for(int i=0;i<noWords;i++){
		printf("%sxxx\n",parsedcmd[i]);
	}
	if (ownCommandHandler(parsedcmd)) return 0;

	pid_t pid,wpid,jpgid = 0;
	pid = fork();
	if(pid<0){
	   fprintf(stderr, "fork failed");
	   return 1;    
	}
	else if(pid == 0){ 
		//handle pipe redirects
        pid_t pidj;
    
        pidj = getpid();
        if (jpgid == 0) jpgid = pidj;
        setpgid(pidj, jpgid);
        tcsetpgrp(shell_terminal, jpgid);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        if(in_fd!=0){
            dup2(in_fd,0);
            close(in_fd);
        }
        if(out_fd!=1){
            dup2(out_fd,1);
            close(out_fd);
        }

        int cmdlen = 0; //len of the command without file redirects
        int flag=0;
        for(int i=0;i<noWords;i++){
            if(!flag){
                if(strcmp(parsedcmd[i],"&")==0||strcmp(parsedcmd[i],"<")==0||strcmp(parsedcmd[i],">")==0){
                    cmdlen=i;
                    flag=1;
                }
            }
            //file redirects
            if(strcmp(parsedcmd[i],">")==0){
                int reOutfd = open(parsedcmd[i+1],O_CREAT | O_TRUNC | O_WRONLY, 0666);
                dup2(reOutfd,STDOUT_FILENO);
            }
            if(strcmp(parsedcmd[i],"<")==0){
                int reInfd = open(parsedcmd[i+1],O_RDONLY);
                dup2(reInfd,STDIN_FILENO);
            }
        }
        if(!flag)
            cmdlen=noWords;
        cmdlen++;
        parsedcmd = (char**)realloc(parsedcmd, sizeof(char*)*cmdlen);
        parsedcmd[cmdlen-1] = NULL;
		execvp(parsedcmd[0],parsedcmd);
	}
	else{
        if (!jpgid) jpgid = pid;
        setpgid(pid, jpgid);
	    if(strcmp(parsedcmd[noWords-1],"&")!=0){
        	int status;
            do{
            	wpid = waitpid(pid,&status,WUNTRACED);
            }
            while(!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        printf("Process excuted successfully\n");
	}
    
    tcsetpgrp(shell_terminal, shell_pgid);
	return 0;
}

int execcmd(char* cmd){
	//parse pipes
	char** pipedcmd;
	int noPipes;
	pipedcmd = (char**)malloc(MAXPIPES*sizeof(char*));
	noPipes = parsePipes(cmd,pipedcmd);
	printf("%d\n",noPipes );

	if (noPipes==1){
		execute(pipedcmd[0],0,1);	
	}
	else if(noPipes>1){
        int i, in_fd = 0;
        int FD[2];//store the read and write file descripters
        for(i = 0; i < noPipes; i++){
            if(pipe(FD)==-1){
                printf("Error: Error in piping\n");
                break;
            }
            else{
                if(i==noPipes-1)
                	execute(pipedcmd[noPipes-1], in_fd, 1); 
                else
                	execute(pipedcmd[i], in_fd, FD[1]);
                close(FD[1]);
                in_fd = FD[0];
            }
        }
    }
}


int main(){
	initShell();
	
	while(1){

		printDirName();

		//char cmd[MAXCMDLEN];
		char *cmd;
		cmd = (char*) malloc(MAXCMDLEN * sizeof(char));
		printf("/myshell$ ");
		
		//if input is blank handle
        memset(cmd, '\0', MAXCMDLEN);
		if(!inputCmd(cmd)) continue;
		
		//executeinput
		execcmd(cmd);

	}
	return 0;
}