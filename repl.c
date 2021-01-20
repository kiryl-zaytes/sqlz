#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COL_USERNAME_SIZE 32
#define COL_EMAIL_SIZE 255
#define size_of_attr(TYPE, ATTR) sizeof(((TYPE*)0)->ATTR) // start counting as it was 0 address + real size of attr;
#define ID_OFFSET 0
#define get_user_offset() ID_OFFSET+ID_SIZE
#define get_email_offset(USER_OFFSET) USER_OFFSET+USERNAME_SIZE
#define get_row_size() ID_SIZE + USERNAME_SIZE + EMAIL_SIZE

typedef struct {
    uint32_t id;
    char username[COL_USERNAME_SIZE];
    char email[COL_EMAIL_SIZE];
} Row;

// Rows representation
const uint32_t ID_SIZE = size_of_attr(Row, id);
const uint32_t USERNAME_SIZE = size_of_attr(Row, username);
const uint32_t EMAIL_SIZE = size_of_attr(Row, email);

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
    IND_SYNTAX_ERROR,
    IND_UNRECOGNIZED
} IndexCommand;

typedef enum {
    INSERT,
    SELECT
} SupportedOperations;

typedef struct {
    SupportedOperations operation;
    Row row_to_insert;
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
        int arg_size = sscanf(ib->buffer, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email);
        if (arg_size < 3) return IND_SYNTAX_ERROR;
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

void serialize_memory_index(Row* source, void* destination){
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + get_user_offset(), &(source->username), USERNAME_SIZE);
    memcpy(destination + get_email_offset(), &(source->email), EMAIL_SIZE);
}

void desirialize_memory_index(void* source, Row* destination){
    memcpy(&(destination->id),source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username),source + get_user_offset(), USERNAME_SIZE);
    memcpy(&(destination->email),source + get_email_offset(),EMAIL_SIZE);
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