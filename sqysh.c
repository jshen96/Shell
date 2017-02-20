#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

struct bgProc{
    pid_t pid;
    char* cmdName;
    struct bgProc* next;
    struct bgProc* prev;
};

struct bgProc* head = NULL;


int insert(pid_t pid, char * cmd){
    struct bgProc* new = malloc(sizeof(struct bgProc));
    if (!new){
        perror("malloc");
        return (-1);
    }
    new->cmdName = cmd;
    new->pid = pid;
    new->next = head;
    head = new;
    return 0;
}

//int removeProc(pid_t pid){
//
//    struct bgProc* curr = head;
//    struct bgProc* previous= head;
//    while (curr!=NULL) {
//        if(pid == curr->pid){
//            if(previous==NULL){
//                head = curr->next;
//            }else{
//                previous->next = curr->next;
//            }
//
//            break;
//        }
//
//        if(previous!=curr){
//            previous=previous->next;
//        }
//        curr = curr->next;
//
//    }
//
//    return 0;
//}
//

char *readLine(FILE* file)
{
    char *line = NULL;
    size_t bufsize = 10;
    if(getline(&line, &bufsize, file)==-1){
        if (file!= stdin) {
            exit(0);
        }
    }
    return line;
}

//char** processCmd(char* inputLine){
//
//	char line[257];
//	strcpy(line,inputLine);
//	char words[150][257];
//	char* word = strtok(line, " \n");
//	int index;
//
//	strcpy(words[0],word);
//
//	for (index=1; word!=NULL; index++) {
//
//		// null will be assign after all words read
//		word = strtok(NULL, " \n");
//		strcpy(words[index],word);
//
//	}
//
//	return words;
//}

int execute(char** arguments, int background){
    pid_t pid;
    pid_t wpid;
    int childStatus;
    char* cmdname = arguments[0];
    
    
    
    pid = fork();
    if (pid==0){
        
        // if pid is 0 it is a child process therefor then execute
        if ( execvp(arguments[0], arguments)==-1){
            // print error
            fprintf(stderr, "%s: %s\n", cmdname, strerror(errno));
            exit(-1);
            
        }
        
    }else if (pid<0){
        printf("fork failed\n");
        
    }else {
        //parent process
        // wait here return the process id from fork at he beggining of this function;
        if(background==1){
            //add process to linked list // trace processes
            insert(pid, cmdname);
        }
        
        if(background!=1){
            wpid = waitpid(pid, &childStatus, 0);
            if(wpid != pid){
                // error
                
            }
        }
    }
    
    return 0;
}



int main(int argc, char** argv){
    int interactive = 0;
    FILE *fp;
    int bckGroundOpt = 0;
    char* cmdName = NULL;
    
    // decide interactive mode or non-interactive mode
    if(argc<=1 && isatty(fileno(stdin))){
        fp = stdin;
        interactive=1;
    }else{
        if(argc<=1) {
            fp = stdin;
        }else{
            fp = fopen( argv[1], "r");
            if (fp==NULL) {
                printf("Cant read file");
                exit(1);
            }
        }
    }
    
    while(1)
    {
        int cExitStatus;
        
        // check for background statuses that are in a different pgid
        // waitpid -1 checks ALL children
        
        
        struct bgProc* curr = head;
        pid_t killedPid;
        while((killedPid = waitpid(killedPid,&cExitStatus, WNOHANG))>0){
            while (curr!=NULL) {
                if(killedPid == curr->pid){
                    fprintf(stderr, "[%s (%d) completed with status %d]\n", curr->cmdName, curr->pid, cExitStatus);
                    // removeProc(curr->pid);
                    break;
                }
                
                curr = curr->next;
            }
        }
        
        
        
        
        // if there isnt a change in status, run the program like normal (WNOHANG)
        // if no zombie processes print prompt
        if (interactive==1 ) {
            printf("%s", "sqysh$ ");
        }else{
            printf("%s", "$ ");
        }
        
        
        // the right file pointer is used
        char* inputLine = readLine(fp);
        char line[257];
        strcpy(line,inputLine);
        free(inputLine);
        char words[150][257];
        char* word = strtok(line, " \n");
        int index;
        strcpy(words[0],word);
        for (index=1; word!=NULL; index++) {
            
            // null will be assign after all words read
            word = strtok(NULL, " \n");
            strcpy(words[index],word);
        }
        
        
        curr = head;
        while((killedPid = waitpid(-1,&cExitStatus, WNOHANG))>0){
            while (curr!=NULL) {
                if(killedPid == curr->pid){
                    fprintf(stderr, "[%s (%d) completed with status %d]\n", curr->cmdName, curr->pid, cExitStatus);
                    // removeProc(curr->pid);
                    break;
                }
                curr = curr->next;
            }
            
        }
        
        
        cmdName = words[0];
        int numCmdArgs = 0;
        char execArg[150][257];
        char* stdinFileName = NULL;
        char* stdoutFileName = NULL;
        mode_t mode = 0644;
        int saveStdIn = dup(0);
        int saveStdOut = dup(1);
        int j = 0;
        
        // loop command arguments to find number of arguments
        for (int i = 0; words[i]!=NULL; i++) {
            numCmdArgs++;
            
            // loop command arguments to find options
            
            if(strcmp(words[i], "<")==0){
                i++;
                stdinFileName = words[i];
                // open file
                int infd =  open(stdinFileName,O_RDONLY);
                if(infd==-1){
                    //error message
                    fprintf(stderr, "%s: %s\n", "open", strerror(errno));
                }
                // dupplicate
                if(dup2(infd, 0)==-1){
                    fprintf(stderr, "%s: %s\n", "dup2", strerror(errno));            }
                close(infd);
                
            }else if (strcmp(words[i], ">")==0){
                i++;
                stdoutFileName = words[i];
                int outfd =  open(stdoutFileName,O_WRONLY|O_CREAT|O_TRUNC, mode);
                if(outfd==-1){
                    //error message
                    fprintf(stderr, "%s: %s\n", "open", strerror(errno));            }
                // duplicte
                if(dup2(outfd, 1)==-1){
                    fprintf(stderr, "%s: %s\n", "dup2", strerror(errno));            }
                
                close(outfd);
            }else if (strcmp(words[i], "&")==0){
                bckGroundOpt=1;
            }else{
                strcpy(execArg[j],words[i]);
                j++;
            }
            
        }
        
        
        if(numCmdArgs>0){
            // if the cmd is any of the built ins, handle
            if( strcmp(cmdName,"cd")==0){
                if (numCmdArgs>2) {
                    fprintf(stderr, "cd: too many arguments\n");
                }else if (numCmdArgs==1){
                    // chdir to home dr
                    chdir(getenv("HOME"));
                }else{
                    char * path = words[1];
                    if(chdir(path)!=0){
                        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
                    }
                }
                
            }else if(strcmp(cmdName,"pwd")==0){
                
                size_t buffSize = 1024;
                char* currDir  = malloc(buffSize);
                while (getcwd(currDir, 1024)==NULL) {
                    buffSize += buffSize;
                }
                printf("%s\n", currDir);
                
            }else if (strcmp(cmdName,"exit")==0){
                //     printf("linked list : \n");
                while (curr!=NULL) {
                    printf("proc : %d\n", curr->pid);
                    curr = curr->next;
                }
                exit(0);
            }else{
                execute((char**)execArg, bckGroundOpt);
            }
            
        }
        bckGroundOpt=0;
        dup2(saveStdIn, 0);
        dup2(saveStdOut, 1);
    }
    
    
    
    
    return 0;
    
}




