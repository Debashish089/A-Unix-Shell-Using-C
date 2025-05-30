#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE 2048
#define MAX_ARGUMENT_SIZE 100
#define HISTORY_SIZE 20

char *history[HISTORY_SIZE];
int history_count = 0;

void sigint_handler(int sig){
    printf("\nI get a signal %d. If you want to quit the shell, write 'exit'.\n", sig);
}

void add_history(char *command){
    if(history_count < HISTORY_SIZE){
        history[history_count] = strdup(command);
        history_count++;
    } else {
        free(history[0]);
        for(int i=0; i<HISTORY_SIZE; i++){
            history[i-1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}
void pipe_command(char *input){
    char cmds[10];
    int num_pipes = 0;
    char *tok = strtok(input, "|");
    while(tok != NULL){
        cmds[num_pipes] = tok;
        tok = strtok(NULL, "|");
        num_pipes++;
    }
    
    int fd[2], prev_fd = 0;
    for(int i=0; i<num_pipes; i++){
        pipe(fd);
        if(fork() == 0){
            dup2(prev_fd, 0);
            if(i < num_pipes  - 1){
                dup2(fd[1], 1);
            }
            close(fd[0]);
            char *args[MAX_ARGUMENT_SIZE];
            char *sub_token = strtok(cmds[i], " ");
            int j=0;
            while(sub_token != NULL){
                args[j] = sub_token;
                sub_token = strtok(NULL, " ");
                j++;
            }
            args[j] = NULL;
            execvp(args[0], args);
            perror("Execution Failed!");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);
        prev_fd = fd[0];
    }
    for(int i=0; i<num_pipes; i++){
        wait(NULL);
    }
}

void command_execution(char **args){
    pid_t pid = fork();
    if(pid == 0){
        if(execvp(args[0], args) == -1){
            perror("Execution failed!");
        }
        exit(EXIT_FAILURE);
    } else if(pid < 0){
        perror("Fork Failed!");
    } else{
        wait(NULL);
    }
}

void print_history(){
    for(int i=0; i<history_count; i++){
        printf("%d : %s\n", i+1, history[i]);
    }
}
void handle_redirection(char *args[]){
    for(int i=0; args[i]!=NULL; i++){
        if(strcmp(args[i] , "<") == 0){
            int fd = open(args[i+1], O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        } else if(strcmp(args[i], ">") == 0){
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if(strcmp(args[i], ">>") == 0){
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

void parsing_executing(char *input){
    add_history(input);
    if(strchr(input, '|')){
        pipe_command(input);
        return;
    }
    
    char *commands[MAX_ARGUMENT_SIZE];
    char *token = strtok(input, ";&");
    int i = 0;
    while(token != NULL){
        commands[i] = token;
        token = strtok(NULL, ";&");
        i++;
    }
    commands[i] = NULL;
    
    for (int j=0; j<i; j++){
        char *args[MAX_ARGUMENT_SIZE];
        char *sub_token = strtok(commands[j], " ");
        int k = 0;
        while(sub_token != NULL){
            args[k] = sub_token;
            sub_token = strtok(NULL, " ");
            k++;
        }
        args[k] = NULL;
        
        if(args[0] != NULL){
            if(strcmp(args[0], "exit") == 0){
                exit(0);
            } else if(strcmp(args[0], "history") == 0){
                print_history();
            } else {
                pid_t pid = fork();
                if(pid == 0){
                    handle_redirection(args);
                    execvp(args[0], args);
                    perror("Execution Failed!!!");
                    exit(EXIT_FAILURE);
                } else {
                    wait(NULL);
                }
            }
        }
    }

}

int main() {

    char input[MAX_INPUT_SIZE];
    signal(SIGINT, sigint_handler);
    while(1){
    
        printf("sh> ");
        if(fgets(input, MAX_INPUT_SIZE, stdin) == NULL){
            printf("There is no input. Please enter inputs.\n");
            continue;
        }
        
        input[strcspn(input, "\n")] = 0;
        parsing_executing(input);
    
    }
    return 0;

}
