#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

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
typedef struct{
    void* pages[MAX_PAGES];
    int file_descriptor;
    uint32_t number_pages;
    uint32_t file_length;
} Pager;

typedef struct {
    Pager* pager;
    uint32_t root_page;
} Table;

typedef struct {
    Table* table;
    uint32_t at_page;
    uint32_t at_cell;
    bool end_of_table;
}Cursor;

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
    IND_TOO_LONG,
    IND_NEGATIVE_ID,
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

/* 
    Each node is one page
*/
typedef enum{
    INTERNAL,
    LEAF
} NodeType;

/*
Header section (some metadata for each node)
*/
#define NODE_SIZE 1
#define NODE_OFFSET 0
#define IS_ROOT_SIZE  1
#define IS_ROOT_OFFSET  1
#define PARENT_POINTER_SIZE  4
#define PARENT_POINTER_OFFSET  2
#define COMMON_NODE_HEADER_SIZE (NODE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

#define LEAF_CELLS_SIZE 4
#define LEAF_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + LEAF_CELLS_SIZE)

/*
Leaf Node Body
*/
const uint32_t LEAF_NODE_KEY_SIZE = 4;
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
#define LEAF_NODE_VALUE_OFFSET (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE

uint32_t* leaf_node_num_cells(void* node) {
  return node + LEAF_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node) { *leaf_node_num_cells(node) = 0; }

void* getPage(Pager *pager, uint32_t page_num){
    if(page_num > MAX_PAGES){
        printf("Page outside of range");
        exit(EXIT_FAILURE);
    }

    if(pager->pages[page_num] == NULL){
        uint32_t current_n = pager->file_length / PAGE_SIZE;
        void* page = malloc(PAGE_SIZE);
        if (pager->file_length % PAGE_SIZE){
            current_n++;
        }
        if (current_n >= page_num){
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytesread = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytesread == -1){
                printf("No bytes to read or error");
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;

        if(page_num >= pager->number_pages){
            pager->number_pages + 1;
        }
    }
    return pager->pages[page_num];
}

void writePage(Pager *pager, uint32_t page_num){
    void* page = pager->pages[page_num];
    if(page == NULL) {
        printf("Nothing to write!!");
        exit(EXIT_FAILURE);
    }
    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1){
        printf("Failed to seek!");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
    if (bytes_written == -1){
        printf("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

Cursor* table_start(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->at_page = table->root_page;
    cursor->at_cell = 0;
    void* root_node = getPage(table->pager, table->root_page);
    uint32_t number_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (number_cells == 0);
    return cursor;
}

Cursor* table_end(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table=table;
    cursor->at_page = table->root_page;
    cursor->end_of_table = true;

    void* root_node = getPage(table->pager, table->root_page);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->at_cell = num_cells;
    return cursor;
}

Pager* pager_open(const char* filename){
    int fd = open(filename, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR);
    if (fd == -1) {
        printf("Cant open file");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length=file_length;
    pager->number_pages = (file_length / PAGE_SIZE);
    if (file_length % PAGE_SIZE != 0) {
        printf("Corrupted page!");
        exit(EXIT_FAILURE);
    }
    int i=0;
    for(i=0; i<MAX_PAGES; i++){
        pager->pages[i] = NULL;
    }
    return pager;
}

void free_table(Pager* pager){
    uint32_t i;
    for(i=0; i < MAX_PAGES; i++){
        free(pager->pages[i]);
    }
    free(pager);
}

void free_memorybuffer(InputBuffer* ib){
    free(ib->buffer);
    free(ib);
}

Table* db_open(const char* filename){
    Pager* pager = pager_open(filename);
    Table* table = malloc(sizeof(Table));
    table->root_page = 0;
    table->pager = pager;
    if (pager->number_pages == 0){
        void* root_node = getPage(pager, 0);
        initialize_leaf_node(root_node);
    }
    return table;
}

void db_close(Table* table){
    Pager* pager = table->pager;

    int i;
    for(i=0; i<pager->number_pages; i++){
        if (pager->pages[i] !=NULL){
            writePage(pager, i);
            free(pager->pages[i]);
            pager->pages[i]=NULL;
        }
    }
    // Edge case: last page could be partial
    // uint32_t rows_leftover = table->n_rows % get_row_size();
    // if(rows_leftover > 0){
    //     writePage(pager, full_pages, rows_leftover * ROW_SIZE);
    //     free(pager->pages[full_pages]);
    //     pager->pages[full_pages]=NULL;
    // }

    int res = close(pager->file_descriptor);
    if(res==-1){
        printf("Problem closing a file");
        exit(EXIT_FAILURE);
    }

    for(i=0; i < MAX_PAGES; i++){
        void* page = pager->pages[i];
        if (page) free(page);
        pager->pages[i] = NULL;
    }

    free(pager);
    free(table);

}

// provide pointer to the very beggining of existing or new page for new 
// row to be inserted
void* cursor_at(Cursor* cursor){
    uint32_t page = cursor->at_page;
    void* ptr = getPage(cursor->table->pager, page);
    //uint32_t offset = cursor->at_row % get_rows_per_page();
    //offset = offset * get_row_size();
    return leaf_node_value(ptr, cursor->at_cell);
}

void cursor_advance(Cursor* cursor){
    uint32_t page_num = cursor->at_page;
    void* node = getPage(cursor->table->pager, page_num);
    cursor->at_cell += 1;
    if (cursor->at_cell >= (*leaf_node_num_cells(node))) { 
        cursor->end_of_table = true;
    }
}

OperationalCommand doOperationalCommand(InputBuffer* ib, Table* table){
    if (strcmp(ib->buffer, ".exit") == 0){
        free_memorybuffer(ib);
        db_close(table);
        exit(EXIT_SUCCESS);
        }
        else{
            return OP_UNRECOGNIZED;
        }
}

IndexCommand isInsertValid(InputBuffer* ib, Statement* statement){
    char* keyword = strtok(ib->buffer, " ");
    char* id_str = strtok(NULL, " ");
    char* user = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if (keyword == NULL || id_str == NULL || user == NULL || email == NULL) return IND_SYNTAX_ERROR;

    int id = atoi(id_str);
    if (id < 0) return IND_NEGATIVE_ID;
    if(strlen(user) > COL_USERNAME_SIZE) return IND_TOO_LONG;
    if(strlen(email) > COL_EMAIL_SIZE) return IND_TOO_LONG;
    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, user);
    strcpy(statement->row_to_insert.email, email);
    return IND_SUCCESS;
}

IndexCommand doIndexCommand(InputBuffer* ib, Statement* statement){
    if (strncmp(ib->buffer, "insert", 6) == 0){
        statement->operation = INSERT;
        // int arg_size = sscanf(ib->buffer, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email);
        // if (arg_size < 3) return IND_SYNTAX_ERROR;
        return isInsertValid(ib, statement);
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

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
    void* node = getPage(cursor->table->pager, cursor->at_page);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
    // Node full
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }
    if (cursor->at_cell < num_cells) {
    // Make room for new cell
        uint32_t i;
        for (i = num_cells; i > cursor->at_cell; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1),
                LEAF_NODE_CELL_SIZE);
        }
    }
    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->at_cell)) = key;
    serialize_memory_index(value, leaf_node_value(node, cursor->at_cell));
}

ExecuteResult execute_insert(Statement* statement, Table* table){
    void* node = getPage(table->pager, table->root_page);

    if ((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS)) {
        return ER_FULL;
    }
    Row* row = &(statement->row_to_insert);
    Cursor* cursor = table_end(table);
    leaf_node_insert(cursor, row->id, row);
    free(cursor);
    return ER_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    uint32_t i;
    Cursor* cursor = table_start(table);

    while(!(cursor->end_of_table)){
        desirialize_memory_index(cursor_at(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }
    free(cursor);
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

    if (argc < 2) {
        printf("Please supply a file name for db!");
        exit(EXIT_FAILURE);
    }
    char* filename = argv[1];
    InputBuffer* ib = new_buffer();
    Table* table = db_open(filename);

    while (true){
        printf("db> ");
        read_input(ib);
        if (ib->buffer[0] == '.'){
            switch (doOperationalCommand(ib, table))
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
        case IND_NEGATIVE_ID:
            printf("Negative ID");
            continue;
        case IND_TOO_LONG:
            printf("Too long command or parameters");
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