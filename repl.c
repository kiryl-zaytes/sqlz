#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COL_USERNAME_SIZE 32
#define COL_EMAIL_SIZE 255
#define COL_ID_SIZE 4
#define size_of_attr(TYPE, ATTR) sizeof(((TYPE*)0)->ATTR) // start counting as it was 0 address + real size of attr;
#define ID_OFFSET 0
#define get_user_offset() (ID_OFFSET+COL_ID_SIZE)
#define get_email_offset(USER_OFFSET)(USER_OFFSET+COL_USERNAME_SIZE)
#define get_row_size() (COL_ID_SIZE + COL_USERNAME_SIZE + COL_EMAIL_SIZE)
#define ROW_SIZE (COL_ID_SIZE + COL_USERNAME_SIZE + COL_EMAIL_SIZE)
#define MAX_PAGES 100
#define PAGE_SIZE 4096
#define get_rows_per_page() (PAGE_SIZE / ROW_SIZE)
#define get_rows_per_table() (MAX_PAGES * get_rows_per_page())


// uint32_t get_offset(){
//     return ID_OFFSET+ID_SIZE;
// }

typedef struct {
    uint32_t n_rows;
    void* pages[MAX_PAGES];
} Table;

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
    ER_SUCCESS,
    ER_FAIL,
    ER_DUPE,
    ER_FULL
} ExecuteResult;

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

Table* new_table(){
    Table* table = malloc(sizeof(Table));
    table->n_rows = 0;
    uint32_t i;
    for(i=0; i < MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table* table){
    uint32_t i;
    for(i=0; i < MAX_PAGES; i++){
        free(table->pages[i]);
    }
    free(table);
}

void free_memorybuffer(InputBuffer* ib){
    free(ib->buffer);
    free(ib);
}

// provide pointer to the very beggining of new existing or new page for new 
// row to be inserted
void* prepare_memory_cell(Table* table, uint32_t row_number){
    uint32_t bucket = row_number / ROW_SIZE;
    void* ptr = table->pages[bucket];
    if (ptr == NULL) { //allocate page
        ptr = table->pages[bucket] = malloc(PAGE_SIZE);
    }
    uint32_t offset = row_number % get_rows_per_page();
    offset = offset * get_row_size();
    return ptr+offset;
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

void print_row(Row* row){
    printf("%d %s %s \n", row->id, row->email, row->username);
}

ExecuteResult execute_insert(Statement* statement, Table* table){
    if (table->n_rows > get_rows_per_table()){
        return ER_FULL;
    }
    Row* row = &(statement->row_to_insert);
    serialize_memory_index(row, prepare_memory_cell(table, table->n_rows));
    table->n_rows+=1;
    return ER_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    uint32_t i;
    for(i=0; i<table->n_rows;i++){
        desirialize_memory_index(prepare_memory_cell(table, i), &row);
        print_row(&row);
    }
    return ER_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table){
    switch (statement->operation){
        case (INSERT):
            return execute_insert(statement, table);
        case (SELECT):
            return execute_select(statement, table);   
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
    // get_offset();

    InputBuffer* ib = new_buffer();
    Table* table = new_table();
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
        switch(execute_statement(&statement, table)){
            case ER_SUCCESS: 
                printf("Executed! \n");
                break;
            case ER_FULL: 
                printf("Table is full!! \n");
                break;
        }
    }
}