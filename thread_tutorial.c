#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMBER_OF_THREADS 5

struct thread_data
{
    long thread_id;
    char *message;
};

struct thread_data array_data[5];

void *PrintHello(void *threadid)
{
    struct thread_data *tid;
    tid = (struct thread_data *)threadid;
    printf("Current id #%ld\n", tid->thread_id);
    printf("Message = > %s \n", tid->message);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    long tid;
    int return_code;
    pthread_t thread_pool[NUMBER_OF_THREADS];

    for (tid = 0; tid < NUMBER_OF_THREADS; tid++)
    {
        printf("Creating thread #%d \n", tid);
        char *buf = (char *)malloc(12 * sizeof(char));
        snprintf(buf, 12, "HELLLOOOO %d", tid); // not thread safe
        array_data[tid].message = buf;
        array_data[tid].thread_id = tid;
        return_code = pthread_create(&thread_pool[tid], NULL, PrintHello, (void *)&array_data[tid]);
        if (return_code)
        {
            printf("ERROR in thread #%ld", tid);
            exit(-1);
        }
    }
    pthread_exit(NULL);
    return 0;
}