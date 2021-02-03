#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define THREADS 3
#define BOUND 10
#define MAX_BUNCH 6

int count = 0;

pthread_mutex_t increment_mutex;
pthread_cond_t threshold;

void *increment(void *tid)
{
    int *iid = tid;
    int i;
    for (i = 0; i < MAX_BUNCH; i++)
    {
        pthread_mutex_lock(&increment_mutex);
        count = count + 1;
        printf("Current count %d \n", count);
        if (count == BOUND)
        {
            pthread_cond_signal(&threshold);
            printf("Butch size for %d reached \n", iid);
            printf("Releasing mutex thread %d \n", iid);
        }
        pthread_mutex_unlock(&increment_mutex);
        sleep(1);
    }
    pthread_exit(NULL);
}

void *watch(void *tid)
{
    int *my_id = (int *)tid;

    pthread_mutex_lock(&increment_mutex);
    while (count < BOUND)
    {
        printf("Waiting for condition to be achieved, tid = %d \n", tid);
        pthread_cond_wait(&threshold, &increment_mutex);
        printf("Condition signal received %d \n", tid);
    }

    count += 100;
    printf("Watcher has added pewww %d \n", tid);
    pthread_mutex_unlock(&increment_mutex);
    pthread_exit(NULL);
}

void main(int argc, int *argv[])
{
    int i, tid;
    int tids[THREADS];
    pthread_t threads[THREADS];
    pthread_attr_t p_attr;
    pthread_mutex_init(&increment_mutex, NULL);
    pthread_cond_init(&threshold, NULL);
    pthread_attr_init(&p_attr);
    pthread_attr_setdetachstate(&p_attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < THREADS - 1; i++)
    {
        tids[i] = i;
        pthread_create(&threads[i], &p_attr, increment, (void *)&tids[i]);
    }
    tids[2] = 2;
    pthread_create(&threads[2], &p_attr, watch, (void *)&tids[2]);
    for (i = 0; i < THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("ALL threads here now %d \n", THREADS);

    pthread_attr_destroy(&p_attr);
    pthread_mutex_destroy(&increment_mutex);
    pthread_cond_destroy(&threshold);
    pthread_exit(NULL);
}