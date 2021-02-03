#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ROOM_CAPACITY 5
#define CHOP 5
#define PORTION_TO_THINK 3
#define PORTION_TO_EAT 3

typedef struct
{
    int pid;
    int health;
    int eat;
    int think;
} Philosopher;

Philosopher *new_person()
{
    return (Philosopher *)malloc(sizeof(Philosopher));
}

pthread_mutex_t chopsticks[CHOP];

void *thinking(void *arg)
{
    int i = 0;
    Philosopher *p = (Philosopher *)arg;
    if (p->health <= 10)
    {
        printf("Philosopher #%d is starving, health %d \n", p->pid, p->health);
        return;
    }

    while (i < PORTION_TO_THINK || p->health > 10)
    {
        p->think += 1;
        p->health -= 2;
        i++;
        printf("Philosopher #%d is thinking! health %d \n", p->pid, p->health);
        sleep(1);
    }
}

void *eating(void *arg)
{
    Philosopher *p = (Philosopher *)arg;
    int i = 0;
    int not_locked;
    while (i < PORTION_TO_EAT)
    {
        i++;
        // pthread_mutex_lock(&chopsticks[p->pid]); // starvation and deadlock
        // pthread_mutex_lock(&chopsticks[(p->pid + 1) % ROOM_CAPACITY]);
        not_locked = pthread_mutex_lock(&chopsticks[p->pid]);
        not_locked = pthread_mutex_trylock(&chopsticks[(p->pid + 1) % ROOM_CAPACITY]);
        if (not_locked)
        {
            pthread_mutex_unlock(&chopsticks[p->pid]);
            pthread_mutex_lock(&chopsticks[(p->pid + 1) % ROOM_CAPACITY]);
            pthread_mutex_lock(&chopsticks[p->pid]);
        }
        p->health += 1;
        p->eat += 1;
        printf("Philosopher #%d is eating, health %d \n", p->pid, p->health);
        sleep(1);
        pthread_mutex_unlock(&chopsticks[p->pid]);
        pthread_mutex_unlock(&chopsticks[(p->pid + 1) % ROOM_CAPACITY]);
    }
}

void *powerOfThought(void *args)
{
    while (1)
        thinking(args), eating(args);
}

void main(int argc, int *argv[])
{
    Philosopher *persons[ROOM_CAPACITY];
    pthread_t philosophers[ROOM_CAPACITY];
    pthread_attr_t attr;
    int pids[ROOM_CAPACITY];

    int i;

    for (i = 0; i < CHOP; i++)
    {
        pthread_mutex_init(&chopsticks[i], NULL);
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (i = 0; i < ROOM_CAPACITY; i++)
    {
        Philosopher *ph = (Philosopher *)new_person();
        ph->pid = i;
        ph->health = 50;
        persons[i] = ph;
    }

    for (i = 0; i < ROOM_CAPACITY; i++)
    {
        pthread_create(&philosophers[i], &attr, *powerOfThought, (void *)persons[i]);
    }

    for (i = 0; i < ROOM_CAPACITY; i++)
    {
        pthread_join(philosophers[i], NULL);
    }
    pthread_exit(NULL);
}
