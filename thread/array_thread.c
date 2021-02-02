#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define TASKS 1000000
#define THREAD_POOL 5
#define BATCH_SIZE (TASKS / THREAD_POOL)

double array[TASKS];
double sum = 0.0;

pthread_mutex_t sum_mutex;

void *doWork(void *tid)
{
    int i;
    int *mytid = (int *)tid;
    int start = (*mytid) * BATCH_SIZE;
    int end = start + BATCH_SIZE;
    double local_sum = 0.0;

    for (i = start; i < end; i++)
    {
        array[i] = i * 1.0;
        local_sum = local_sum + array[i];
    }
    printf("Thread #%d is working now\n", *mytid);
    pthread_mutex_lock(&sum_mutex);
    printf("Thread #%d in mutex\n", *mytid);
    sum = sum + local_sum;
    pthread_mutex_unlock(&sum_mutex);
    pthread_exit(NULL);
}

void main(int argc, int *argv[])
{
    pthread_t thread_pool[THREAD_POOL];
    int pids[THREAD_POOL];
    pthread_attr_t attr;

    int i;

    pthread_mutex_init(&sum_mutex, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < THREAD_POOL; i++)
    {
        pids[i] = i;
        pthread_create(&thread_pool[i], &attr, doWork, (void *)&pids[i]);
    }

    for (i = 0; i < THREAD_POOL; i++)
    {
        pthread_join(thread_pool[i], NULL);
    }

    printf("Sum is %f", sum);
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&sum_mutex);
    pthread_exit(NULL);
}