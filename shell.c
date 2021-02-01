#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MAX_TOKENS 20
#define DELIM " \t\n\r"
#define PATH_MAX 64

typedef struct {
    char* str;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer* new_buffer(){
    InputBuffer* ib = malloc(sizeof(InputBuffer));
    return ib;
}


int internal_cd(char** path);
int internal_exit(char** arg);
int internal_run(char** args);

char* predifined_commands[] = {
    "cd",
    "exit"
};

int (*function_pointers[]) (char**) = {
    &internal_cd,
    &internal_exit
};

int make_system_call(char* path){
    printf("%d\n", SYS_chdir);
    syscall(SYS_chdir, 80, path, 12);
    char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf(cwd);
    }
}

int internal_cd(char** args){
    if(args[1] == NULL){
        printf("Path not provided!");
    }
    else{
        make_system_call(args[1]);
        if (chdir(args[1]) !=0 ){
            perror("shell");
        }
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf(cwd);
        }
    }
    return 1;
}

int internal_exit(char** args){
    return 0;
}

char** tokenize(char* str){
    int i = 0;
    char** tokens = malloc(MAX_TOKENS * sizeof(char));
    char* token;
    token = strtok(str, DELIM);

    while(token != NULL){
        tokens[i] = token;
        i++;
        token=strtok(NULL, DELIM);
    }
    tokens[i] = NULL;
    return tokens;
}

int run_pipe(char* cmd){
    int descriptors[2];
    int p = pipe(descriptors);
    pid_t pid_c1, pid_c2;
    char* token;

    token = strtok(cmd, "|");
    char** tokens = malloc(MAX_TOKENS * sizeof(char));
    int i = 0;
    while(token != NULL){
        tokens[i] = token;
        token = strtok(NULL, "|");
        i++;
    }
    tokens[i] = NULL;

    char** storage[i][1];
    int j=0;
    while (j < i)
    {
        storage[j][0] = tokenize(tokens[j]);
        j++;
    }
    free(tokens);
    int status1, status2;
    switch((pid_c1 = fork())){
        case -1: 
            perror("fork");
            break;
        case 0:
            printf("Getting from %d \n", getpid());
            dup2(descriptors[1], STDOUT_FILENO);
            close(descriptors[0]);
            close(descriptors[1]);
            if(execvp(*(storage[0])[0], *(storage[0])) == -1) perror("shell 1");
            exit(1);
    };
    switch((pid_c2 = fork())){
        case -1: 
            perror("fork");
            break;
        case 0:
            printf("Getting from %d \n", getpid());
            dup2(descriptors[0], STDIN_FILENO);
            close(descriptors[1]);
            close(descriptors[0]);
            if(execvp(*(storage[1])[0], *(storage[1])) == -1) perror("shell 2");
            exit(1);
    };
    close(descriptors[0]);
    close(descriptors[1]);
    waitpid(pid_c1, &status1, WUNTRACED);
    waitpid(pid_c2, &status2, WUNTRACED);
    return 1;
}

int run_command(char** args){
    pid_t wpid;
    pid_t pid = fork();
    int status;
    if (pid == 0){
        printf("Spawning new process");
        if(execvp(args[0], args) == -1){
            printf("Error running new process");
            perror("shell");
        }
        exit(EXIT_FAILURE);
    }
    else if(pid < 0){
        perror("shell");
    }
    else{
        while (!WIFEXITED(status) && !WIFSIGNALED(status))
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        }
        
    }
    return 1;
}

int internal_run(char** args){
    int i;
    if(args[0] == NULL) {
        return 1;
    }
    int ll = sizeof(predifined_commands) / sizeof(char *);
    for(i=0; i<ll;i++){
        if (strcmp(args[0], predifined_commands[i]) == 0){
            return function_pointers[i](args);
        }
    }
    return run_command(args);
}

int main(int argc, char* argv){
    InputBuffer* ib = new_buffer();
    int exit_code = 1;

    while (exit_code){
        printf(">");
        ssize_t bytes = getline(&(ib->str), &(ib->buffer_length), stdin);
        ib->buffer_length = bytes-1;
        ib->str[bytes-1] = 0;
        if(bytes < 0) exit(EXIT_FAILURE);
        if (strstr(ib->str, "|") != NULL) run_pipe(ib->str);
        else{
            char** params = tokenize(ib->str);
            exit_code = internal_run(params);
        };
        free(ib);
    }
    return EXIT_SUCCESS;
}