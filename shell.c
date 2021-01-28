#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
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

char* predifined_commands[] = {
    "cd",
    "exit"
};

int (*function_pointers[]) (char**) = {
    &internal_cd,
    &internal_exit
};

int internal_cd(char** args){
    if(args[1] == NULL){
        printf("Path not provided!");
    }
    else{
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
    int exit_code;

    while (exit_code){
        printf(">");
        ssize_t bytes = getline(&(ib->str), &(ib->buffer_length), stdin);
        ib->buffer_length = bytes-1;
        ib->str[bytes-1] = 0;
        if(bytes < 0){
            exit(EXIT_FAILURE);
        }
        char** params = tokenize(ib->str);
        int exit_code = internal_run(params);
        free(ib);
    }
    return EXIT_SUCCESS;
}