#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/inotify.h>

#define MAXCMDLEN 200
#define MAXCMDNUM 50
#define MAXPIPES 20

#define HISTFILESIZE 10000
#define HISTSHOWSIZE 1000

#define MAXMULTIWATCHCMDS 100

pid_t shell_pgid;
int shell_terminal, shell_is_interactive;
struct termios shell_tmodes;

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
			printf("\n");
			str[i] = '\0';
			break;
		}
		else if(c=='\t'){
			//searching for file will be handled
			temp[j]=='\0';
			printf("\n%s\n",temp );
			// frint options here
			//matches = 

			sleep(2);


			write(STDOUT_FILENO, "\033[F", 5);
			write(STDOUT_FILENO, "\033[K", 5);
			printf("\b \b");
			write(STDOUT_FILENO, "\033[1A", 6);
			write(STDOUT_FILENO, "\033[1A", 6);
			printDirName();
			printf("/myshell$ ");
			strcat(str,temp);
			printf("%s", str);

			free(temp);
			temp = (char*)malloc(sizeof(char)*50);
			j=0;
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

int handleMultiwatch(char** parsed){
    int noOfCmds = 0;


    char **commands = (char **)malloc(sizeof(char *)*MAXMULTIWATCHCMDS);
    if (!commands){
        fprintf(stderr, "Memory initialization error\n");
    }
    while (parsed[++noOfCmds] != NULL){
        commands[noOfCmds-1] = (char *)malloc(sizeof(char)*MAXCMDLEN);
        memcpy(commands[noOfCmds - 1], &parsed[noOfCmds][1], strlen(parsed[noOfCmds]) - 2);
    }
    noOfCmds--;

    int fd = inotify_init();
    int inotify_id[noOfCmds];
    int readFds[noOfCmds];  



    for (int i=0;i<noOfCmds;i++){
        printf("in handle: %s", commands[i]);
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
            while(1){
                execute(commands[i], 0, fd);
                sleep(2);
                printf("Updated output for process of PID %d", pidj);
            }

        }
        else{
            if (!jpgid) jpgid = pid;
            setpgid(pid, jpgid);
            char fileName[50];
            sprintf(fileName, ".temp.%d.txt", pid);
            int fd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);
            int wd = inotify_add_watch(fd, fileName, IN_MODIFY);
        }
    }

    return 1;
}

// int shell_multiWatch(char *line){

//     //signal(SIGTSTP, multiWatch_ctrl_c);
//     // signal(SIGINT, multiWatch_ctrl_c);
//     char **cammands;
//     cammands = (char**)malloc(sizeof(char *)*BUF_LEN);
//     if (!cammands) 
//     {
//         memory_failed_error();
//     }

//     char *temp;
//     temp = strtok(line,"\"");
//     temp = strtok(NULL,"\"");
//     int size=0;
//     while(temp!=NULL){
//         cammands[size] = temp;
//         //printf("%s\n",temp);
//         size++;
//         temp = strtok(NULL,"\"");
//         temp = strtok(NULL,"\"");
        
//     }
//     cammands[size] = NULL;
//     char ***arguments;
//     arguments = (char**)malloc(sizeof(char *)*size+1);
//     int positions[size];

//     int fd = inotify_init();
//     int inotify_id[size];
//     int readFds[size];
    
//     for(int i=0;i<size;i++){
//         //printf("%s\n",cammands[i]);
//         //arguments[i] = cammands[i];
//         char **args;
//         int position = 0;
//         args = shell_split_line(cammands[i], &position);
//         arguments[i] = args;
//         positions[i] = position;
        
//         char *str;
//         str = malloc(sizeof(char)*BUF_LEN);
//         for(int i=0;i<BUF_LEN;i++)
//             str[i] = '\0';
//         sprintf(str, ".temp.PID%d.txt", i+1);
//         int discriptor = open(str,O_CREAT | O_APPEND, 0666);
//         readFds[i] = discriptor;
//         //close(discriptor);
//         //watchFiles(str,cammands[i]);
//         int wd = inotify_add_watch( fd, str, IN_MODIFY);
//         inotify_id[i] = wd;
//     }
//     pid_t pid_to_console = fork();

//     if(pid_to_console==0){
//         //signal(SIGTSTP, SIG_DFL);
//         signal(SIGINT, SIG_DFL);
//         char buf[EVENT_BUF_LEN]
//                     _attribute((aligned(alignof_(struct inotify_event))));
//         for(int i=0;i<EVENT_BUF_LEN;i++) buf[i] = '\0';
//         while(1){    
//             int len = read( fd, buf, EVENT_BUF_LEN );
//             if(len<=0) perror("read error");
//             int i=0;
//             while(i<len){
//                 struct inotify_event *event = (struct inotify_event *)&buf[i];
                
//                 if(event->mask & IN_MODIFY){

//                     int wd = event->wd;
//                     int fileIndex =0;
//                     for(int j=0;j<size;j++){
//                         if(wd==inotify_id[j]){
//                             fileIndex = j;
//                             break;
//                         }
//                     }
//                     time_t t;   // not a primitive datatype
//                     time(&t);
//                     char read_buf[BUF_LEN + 1];
//                     for (int i = 0; i <= BUF_LEN; i++) {
//                         read_buf[i] = '\0';
//                     }
                    
//                     fprintf(stdout,"\"%s\", %s\n<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n",cammands[fileIndex],ctime(&t));
//                     while (read(readFds[fileIndex], read_buf, BUF_LEN) > 0) {
//                         fprintf(stdout, "%s", read_buf);
//                         for (int i = 0; i <= BUF_LEN; i++) {
//                             read_buf[i] = '\0';
//                         }
//                     }
//                     fprintf(stdout, "->->->->->->->->->->->->->->->->->->->->\n\n");
//                 }
//                 i += sizeof(struct inotify_event) + event->len;
//             }
//         }
//     }

//     while(multiWatch_c_detect){
//         for(int i=0;i<size;i++){

//             //char **args;
//             //int position = 0;
//             //args = shell_split_line(cammands[i], &position);
//             //arguments[i] = args;
//             //positions[i] = position;

//             char *str;
//             str = malloc(sizeof(char)*BUF_LEN);
//             sprintf(str, ".temp.PID%d.txt", i+1);
//             int discriptor = open(str,O_CREAT| O_WRONLY | O_APPEND, 0644);
//             //printf("%d\n",positions[i]);
//             shell_launch(arguments[i],positions[i],0,discriptor,0);
//         }
//         sleep(1);

        

        
        
//         //char final[EVENT_BUF_LEN+1000];
//         //for(int i=0;i<size;i++)
//         //sprintf(final,"\"%s\", %s\n<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n%s\n->->->->->->->->->->->->->->->->->->->\n",cammand,ctime(&t),buffer);
//         //printf("%s",final);


//     }
//     multiWatch_c_detect=1;
    
//     return 1;
// }


void handleMultiWatch(char** parsed){
    int i = 0;

    int fds[MAXMULTIWATCHCMDS];
    int noOfCmds = 0;

    fd_set rset;
    int maxfd = 0;
    FD_ZERO(&rset);

    while (parsed[++i] != NULL){
        char *cmd;
		cmd = (char*) malloc(MAXCMDLEN * sizeof(char));
		strcpy(cmd, "watch ");
        strcat(cmd, parsed[i]);
        printf("Multiwatch command %d: %s\n", i, cmd);
        
        int len = strlen(cmd);
        while(cmd[len-1]==' '){
            len--;
        }
        cmd[len] = '\0';
        char** parsedcmd;
        parsedcmd = (char**)malloc(MAXCMDNUM*sizeof(char*));
        int noWords = parseCmd(cmd, parsedcmd);
        if (ownCommandHandler(parsedcmd)) return;

        pid_t pid,wpid,jpgid = 0;
        pid = fork();
        if (pid < 0){
            fprintf(stderr, "fork failed");
            return;
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

            int cmdlen = 0; //len of the command without file redirects
            int flag=0;
            for(int i=0;i<noWords;i++){
                if(!flag){
                    if(strcmp(parsedcmd[i],"&")==0||strcmp(parsedcmd[i],"<")==0||strcmp(parsedcmd[i],">")==0){
                        cmdlen=i;
                        flag=1;
                    }
                }
            }
            if(!flag)
                cmdlen=noWords;
            cmdlen++;
            parsedcmd = (char**)realloc(parsedcmd, sizeof(char*)*cmdlen);
            parsedcmd[cmdlen-1] = NULL;

            char fileName[50];
            sprintf(fileName, ".temp.%d.txt", pidj);
            int reOutfd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);
            dup2(reOutfd, 1);
            printf("%s", cmd);
            close(reOutfd);
    		execvp(parsedcmd[0],parsedcmd);
        }
        else{
            if (!jpgid) jpgid = pid;
            setpgid(pid, jpgid);
            char fileName[50];
            printf("%d\n", pid);
            sprintf(fileName, ".temp.%d.txt", pid);
            int reInfd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);

            FD_SET(reInfd, &rset);
            fds[noOfCmds++] = reInfd;
            printf("Created file descriptor: %d\n", reInfd);
            maxfd = maxfd > reInfd ? maxfd : reInfd;
        }
    }

    // printf("Outside %d\n", noOfCmds);
    // for (int i=0;i<noOfCmds;i++){
    //     printf("%s ",fds[i]);
    // }
    // while (1){
        // char buffer[1000];
        // int fd = open(fds[0], O_RDWR);
        // printf("%d \n",fd);
        // int fd = open(fds[0], O_RDONLY);
        // read(fd, buffer, 1000);
        // printf("%s",buffer);
        // sleep(2000);
    // }

    // int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
    // printf("%d ", nready);
    // if (nready > )
    // for 

    // while(1){
        if (select(maxfd + 1, &rset, NULL, NULL, NULL)){
            for (int i = 0; i < noOfCmds; i++ ){
                if (FD_ISSET(fds[i], &rset)){
                    char buffer[1000];
                    printf("%d \n", fds[i]);
                    int n = read(fds[i], buffer, 1000);
                    if (n > 0){
                        buffer[n] = '\0';
                        printf("%s\n", buffer);
                    }
                    else{
                        printf("Error while reading.. \n");
                    }
                }
            } 
        }
    // }
}

int ownCommandHandler(char** parsed){
	int NoOfOwnCmds = 5, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
    ListOfOwnCmds[4] = "multiwatch";
  
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
    case 5:
        handleMultiwatch(parsed);
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