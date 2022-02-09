#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <time.h>

#define MAXCMDLEN 200
#define MAXCMDNUM 50
#define MAXPIPES 20
#define MAXMATCHES 100

#define HISTFILESIZE 10000
#define HISTSHOWSIZE 1000

#define MAXMULTIWATCHCMDS 100
#define EVENT_BUF_LEN 100
#define READ_BUF_LEN 100

pid_t shell_pgid;
int shell_terminal, shell_is_interactive;
struct termios shell_tmodes;
char* storehistory[10001];
int hiscount;


int multi_watch_called = 0;
char *multiWatchFds[MAXMULTIWATCHCMDS];
int multiWatchPIDs[MAXMULTIWATCHCMDS];


int max(int a,int b){
	if(a>b) return a;
	return b;
}

//utility functions: getch implementation
static struct termios old, current;

int execcmd(char* cmd);
int ownCommandHandler(char** parsed);
int execute(char* cmd,int in_fd,int out_fd);

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
//another set of utility functions: substring matching
void lcsmatch(char* text, char* lcsp){
	for (int i = 0; i < strlen(lcsp); ++i){
		if(i>strlen(text)){
			lcsp[i] = '\0';
			return;
		}
   		if(lcsp[i]==text[i])	continue;
   		else{
   			lcsp[i] = '\0';
   			break;
   		}
   }
}

int isPrefix(char* text, char* pattern) {
   int m = strlen(pattern);
   int n = strlen(text);
   if(m>n) 	return 0;
   for (int i = 0; i < m; ++i){
   		if(pattern[i]==text[i])	continue;
   		else	return 0;
   }
   return 1;
}

int lcs(char* text, char* pattern){
	int m = strlen(pattern);
   	int n = strlen(text);
	int **dp;
	dp = (int**)malloc(sizeof(int*)*(m+1));
	for (int i = 0; i <= m; ++i){
		dp[i] = (int*)malloc(sizeof(int)*(n+1));
	}

    int count = 0; 
    for (int i = 0; i <= m; i++){
        for (int j = 0; j <= n; j++){
            if (i == 0 || j == 0)
                dp[i][j] = 0;
 
            else if (text[i - 1] == pattern[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
                count = max(count, dp[i][j]);
            }
            else
                dp[i][j] = 0;
        }
    }
    return count;
}
//utility function: custom strtok
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



void initShell(){
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    //read from history file
    FILE *fp;
    if((fp=fopen("history.txt","r"))==NULL){
    	fp = fopen("history.txt","w");
    	fclose(fp);
    }
    else{
    	char buf[MAXCMDLEN];
    	while((fgets(buf,MAXCMDLEN,fp))!=NULL ){
    		storehistory[hiscount] = strdup(buf);
    		hiscount++;
    	}
    	fclose(fp);
    }

   

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


void sigtstp_handler(int sig_num){
	signal(SIGTSTP, sigtstp_handler);

	printf("Got a Ctrl+Z interrupt\n");
}

void add_history(char *command){
	hiscount %= 10000;
	char *str;
	str = strdup(command);
	strcat(str,"\n");
	storehistory[hiscount] = str;
	hiscount++;
}

void update_history(){
	FILE* fp;
	if((fp=fopen("history.txt","w"))){
		for (int i = 0; i < hiscount; ++i){
			fputs(storehistory[i],fp);
		}
	}
}

int file_matches(char* temp, char** matches){
	DIR *d;
    struct dirent *dir;
    d = opendir(".");
    int count=0;
    if (d){
        while ((dir = readdir(d)) != NULL){
        	if(strcmp(".",dir->d_name)==0 || strcmp("..",dir->d_name)==0) continue;
        	if(isPrefix(dir->d_name,temp))
        		matches[count++] = dir->d_name;
        }
        closedir(d);
    }
    return count;
}


int inputCmd(char* str){
	int i=0,j=0;
	char c, *temp;
	temp = (char*)malloc(sizeof(char)*50);
	
	while(1){
		c = getch();
		if(c=='\n'){
			printf("\n");
			str[i] = '\0';
			break;
		}
		else if(c=='\033'){
			getch();
			getch();
			continue;
		}
		else if(c=='\t'){
			//searching  file will be handled
			temp[j]=='\0';
			// frint options here
			char **matches;
			matches = (char**)malloc(sizeof(char*)*MAXMATCHES);
			int noMatches = file_matches(temp,matches);
			
			//autocomplete
			if(noMatches==0) continue;
			else if(noMatches==1){
				for(int fn=strlen(temp);fn<strlen(matches[0]);fn++){
					printf("%c",matches[0][fn] );
					str[i++] = matches[0][fn];
				}

				free(temp);
				temp = (char*)malloc(sizeof(char)*50);
				j=0;

			}
			else{
				char *lcsp;
				lcsp = strdup(matches[0]);
				for (int file = 0; file < noMatches; ++file)
					lcsmatch(matches[file],lcsp);
				for (int cn = strlen(temp); cn < strlen(lcsp); ++cn)
					printf("%c",lcsp[cn] );
				printf("\nchoose among: ");
				for (int file = 0; file < noMatches; ++file)
					printf("%d.%s  ,  ",file,matches[file] );

				printf("\n");
				char inp=getch();
				int idx = inp-'0';

				for(int fn=strlen(temp);fn<strlen(matches[idx]);fn++){
					str[i++] = matches[idx][fn];
				}
				write(STDOUT_FILENO, "\033[F", 5);
				write(STDOUT_FILENO, "\033[K", 5);
				printf("\b \b");
				write(STDOUT_FILENO, "\033[F", 5);
				write(STDOUT_FILENO, "\033[K", 5);
				printDirName();
				printf("/myshell$ ");
				//strcat(str,temp);
				printf("%s", str);

				free(temp);
				temp = (char*)malloc(sizeof(char)*50);
				j=0;
			}

		}
		//implement backspace
		else if(c==127){
			if(i>0){
				printf("%c",c);
				printf("\b \b");
				str[--i] = '\0';
				if(j)	temp[--j] = '\0';
			}
		}
		else{
			printf("%c",c );
			str[i++] = c;
			temp[j++] = c;
		}
		if(c==' '){
			free(temp);
			temp = (char*)malloc(sizeof(char)*50);
			j=0;
		}		
		
	}
	return i;
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
        "\nList of Commands supported:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>all other general commands available in UNIX shell"
        "\n>pipe handling"
        "\n>improper space handling");
  
    return;
}

void endmultiWatch(int signum){
    signal(SIGINT, SIG_IGN);
    multi_watch_called = 0;

    int i = 0;
    while (multiWatchFds[i] != NULL){
        remove(multiWatchFds[i]);
        kill(multiWatchPIDs[i], SIGKILL);
        i++;
    }

}

int handleMultiwatch(char** parsed){
    int noOfCmds = 0;
    signal(SIGINT, endmultiWatch);
    multi_watch_called = 1;

    char **commands = (char **)malloc(sizeof(char *)*MAXMULTIWATCHCMDS);
    if (!commands){
        fprintf(stderr, "Memory initialization error\n");
    }
    while (parsed[++noOfCmds] != NULL){
        commands[noOfCmds-1] = (char *)malloc(sizeof(char)*MAXCMDLEN);
        memcpy(commands[noOfCmds - 1], &parsed[noOfCmds][1], strlen(parsed[noOfCmds]) - 2);
    }
    noOfCmds--;

    int fd_notify = inotify_init();
    int inotify_id[noOfCmds];
    int readFds[noOfCmds];  
    memset(multiWatchFds, 0, MAXMULTIWATCHCMDS);

    for (int i=0;i<noOfCmds;i++){
        char *cmd = commands[i];

        int len = strlen(cmd);
        while(cmd[len-1]==' '){
            len--;
        }
        cmd[len] = '\0';
        printf("%s\n", cmd);
        char** parsedcmd;
        parsedcmd = (char**)malloc(MAXCMDNUM*sizeof(char*));
        int noWords = parseCmd(cmd, parsedcmd);

        pid_t jpgid = 0;
        pid_t pid = fork();
        if (pid < 0){
            fprintf(stderr, "Error in creating child process\n");
        }
        else if (pid == 0){
            pid_t pidj;
            pidj = getpid();
            if (jpgid == 0) jpgid = pidj;
            setpgid(pidj, jpgid);

            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            char fileName[50];
            sprintf(fileName, ".temp.%d.txt", pidj);
            int fd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);

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
            int test = 0;
            dup2(fd, STDOUT_FILENO);
            close(fd);
            while(multi_watch_called){
                sleep(1);
                pid_t pid_child, wpid;
                pid_child = fork();
                if (pid_child == 0){
                    execvp(parsedcmd[0], parsedcmd);
                }
                else if (pid_child > 0){
                    if(strcmp(parsedcmd[noWords-1],"&")!=0){
                        int status;
                        do{
                            wpid = waitpid(pid,&status,WUNTRACED);
                        }
                        while(!WIFEXITED(status) && !WIFSIGNALED(status));
                    }
                    printf("Process excuted successfully\n");
                    continue;
                }
            }

        }
        else{
            if (!jpgid) jpgid = pid;
            setpgid(pid, jpgid);
            char *fileName;
            fileName = (char *)malloc(50*sizeof(char));
            sprintf(fileName, ".temp.%d.txt", pid);
            int fd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);
            int wd = inotify_add_watch(fd_notify, fileName, IN_MODIFY);
            inotify_id[i] = wd;
            readFds[i] = fd;
            multiWatchFds[i] = fileName;
            multiWatchPIDs[i] = pid;
        }
    }

    pid_t wpid_to_console, jpgid_to_console;
    pid_t pid_to_console = fork();

    if (pid_to_console == 0){
        pid_t pidj;
    
        pidj = getpid();
        if (jpgid_to_console == 0) jpgid_to_console = pidj;
        setpgid(pidj, jpgid_to_console);
        tcsetpgrp(shell_terminal, jpgid_to_console);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        char buf[EVENT_BUF_LEN];
        memset(buf, '\0', EVENT_BUF_LEN);
        while(1){
            int len = read(fd_notify, buf, EVENT_BUF_LEN);
            if (len <= 0) perror("Read Error");
            int i = 0;
            while (i < len){
                struct inotify_event *event = (struct inotify_event *)&buf[i];

                if (event->mask & IN_MODIFY){
                    int wd = event->wd;
                    int fileIdx = 0;
                    for (int j=0;j<noOfCmds;j++){
                        if (wd == inotify_id[j]){
                            fileIdx = j;
                            break;
                        }
                    }

                    time_t t;
                    time(&t);
                    char read_buf[READ_BUF_LEN];
                    memset(read_buf, '\0', READ_BUF_LEN);
                    fprintf(stdout, "\"%s\", %s\n<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n", commands[fileIdx], ctime(&t));
                    while(read(readFds[fileIdx], read_buf, READ_BUF_LEN) > 0){
                        fprintf(stdout, "%s", read_buf);
                        memset(read_buf, '\0', READ_BUF_LEN);
                    }
                    fprintf(stdout, "->->->->->->->->->->->->->->->->->->->->\n\n");
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
    }
    else if (pid_to_console > 0){
        if (!jpgid_to_console) jpgid_to_console = pid_to_console;
        setpgid(pid_to_console, jpgid_to_console);
        int status;
        do{
            wpid_to_console = waitpid(pid_to_console,&status,WUNTRACED);
        }
        while(!WIFEXITED(status) && !WIFSIGNALED(status));
        printf("Process excuted successfully\n");
    }

    tcsetpgrp(shell_terminal, shell_pgid);
    return 1;
}


void search_history(){
	printf("Travel your history: \n");
	for (int i = 0; i < hiscount; ++i){
		printf("%s", storehistory[i]);
	}
	if(getch()==18){
		printf("Enter search term: ");
		char *temp;
		int lcslen, found=0,idx=0;
		char *closematch[HISTSHOWSIZE];
		temp = (char*)malloc(sizeof(char)*MAXCMDLEN);
		scanf("%[^\n]",temp);
		//printf("Matched commands are:\n");
		for (int i = hiscount-1; i>=0;--i){
			lcslen = lcs(storehistory[i],temp);
			if (lcslen==strlen(temp)){
				printf("%s", storehistory[i]);
				found = 1;
			}
			else if(lcslen>2){
				closematch[idx++] = storehistory[i];
			}
		}
		if(!found){
			for (int i = 0; i < idx; ++i){
				printf("%s", closematch[i]);
			}
		}
		if(!found && idx==0)	
			printf("No match for search term in history\n");
		free(temp);
	}
	printf("\n");
}

int ownCommandHandler(char** parsed){
	int NoOfOwnCmds = 6, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
    ListOfOwnCmds[4] = "multiwatch";
    ListOfOwnCmds[5] = "history";
  
    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }
	
    switch (switchOwnArg) {
    case 1:
    	update_history();
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
        printf("\nHello %s.\nPut your command to see magic"
            "\nUse help to know more..\n",
            username);
        return 1;
    case 5:
        handleMultiwatch(parsed);
        return 1;
    case 6:
    	search_history();
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
	/*for(int i=0;i<noWords;i++){
		printf("%sxxx\n",parsedcmd[i]);
	}
	*/
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
		printf("Error in command! exiting....\n");
		kill(pid, SIGKILL);
		return EXIT_FAILURE;
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
	return EXIT_SUCCESS;
}

int execcmd(char* cmd){
	//parse pipes
	char** pipedcmd;
	int noPipes;
	pipedcmd = (char**)malloc(MAXPIPES*sizeof(char*));
	noPipes = parsePipes(cmd,pipedcmd);
	//printf("%d\n",noPipes );

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
	hiscount = 0;
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
		add_history(cmd);
		//executeinput
		execcmd(cmd);
		free(cmd);

	}
	
	return 0;
}