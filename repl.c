#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    OP_SUCCES,
    OP_UNRECOGNIZED
} OperationalCommand;

typedef enum {
    IND_SUCCESS,
    IND_UNRECOGNIZED
} IndexCommand;

typedef enum {
    INSERT,
    SELECT
} SupportedOperations;

typedef struct {
    SupportedOperations operation;
} Statement;

void free_memorybuffer(InputBuffer* ib){
    free(ib->buffer);
    free(ib);
}

OperationalCommand doOperationalCommand(InputBuffer* ib){
    if (strcmp(ib->buffer, ".exit") == 0){
        free_memorybuffer(ib);
        exit(EXIT_SUCCESS);
        }
        else{
            return OP_UNRECOGNIZED;
        }
}

IndexCommand doIndexCommand(InputBuffer* ib, Statement* statement){
    if (strncmp(ib->buffer, "insert", 6) == 0){
        statement->operation = INSERT;
        return IND_SUCCESS;
    }
    if(strncmp(ib->buffer, "select", 6) == 0){
        statement->operation = SELECT;
        return IND_SUCCESS;
    }
    return IND_UNRECOGNIZED;
}

void exexute_statement(Statement* statement){
    switch (statement->operation){
        case (INSERT):
            printf("Executing insert operation");
            break;
        case (SELECT):
            printf("Executing select operation");
            break;    
    }
}

InputBuffer* new_buffer(){
    InputBuffer* input_buffer=malloc(sizeof(InputBuffer));
    return input_buffer;
}

void read_input(InputBuffer* ib){
    ssize_t bytes_read=getline(&(ib->buffer), &(ib->buffer_length), stdin);
    if (bytes_read <=0){
        printf("Error reading input");
        exit(EXIT_FAILURE);
    }
    ib->input_length = bytes_read-1;
    ib->buffer[bytes_read-1] = 0;

}

void main(int argc, char* argv[]){
    InputBuffer* ib = new_buffer();
    while (true){
        printf("db> ");
        read_input(ib);
        if (ib->buffer[0] == '.'){
            switch (doOperationalCommand(ib))
            {
            case OP_SUCCES:
                continue;
            case OP_UNRECOGNIZED: 
                printf("Unsupported command %s ", ib->buffer);
                continue;
            }
        }

        Statement statement;
        switch (doIndexCommand(ib, &statement))
        {
        case IND_SUCCESS:
            break;
        case IND_UNRECOGNIZED:
            printf("Unrecognized db operation %s", ib->buffer);
            continue;
        }
        exexute_statement(&statement);
    }
}