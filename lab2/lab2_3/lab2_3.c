#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int flag1 = 0, flag2 = 0;
pthread_spinlock_t spin;

void* proc1(void* args)
{
    int ret;

    while (!flag1)
    {
        while ((ret = pthread_spin_trylock(&spin)) != 0)
        {
            if (ret == EBUSY) 
            {
                printf("\nПоток 1: спинлок занят\n");
                usleep(1000000);
            } 
            else 
            {
                fprintf(stderr, "pthread_spin_trylock (proc1): %s\n", strerror(ret));
                pthread_exit(NULL);
            }

            if (usleep(100000) == -1) 
            {
                perror("usleep");
            }
        }

        for (int i = 0; i < 10; ++i)
        {
            printf("1");
            fflush(stdout);

            if (sleep(1) == -1) 
            {
                perror("sleep");
            }
        }

        ret = pthread_spin_unlock(&spin);
        if (ret != 0) 
        {
            fprintf(stderr, "pthread_spin_unlock (proc1): %s\n", strerror(ret));
            pthread_exit(NULL);
        }

        if (sleep(1) == -1) 
        {
            perror("sleep");
        }
    }

    return NULL;
}

void* proc2(void* args)
{
    int ret;

    while (!flag2)
    {
        while ((ret = pthread_spin_trylock(&spin)) != 0)
        {
            if (ret == EBUSY) 
            {
                printf("\nПоток 2: спинлок занят\n");
                usleep(1000000);
            } 
            else 
            {
                fprintf(stderr, "pthread_spin_trylock (proc2): %s\n",strerror(ret));
                pthread_exit(NULL);
            }

            if (usleep(100000) == -1) 
            {
                perror("usleep");
            }
        }

        for (int i = 0; i < 10; ++i)
        {
            printf("2");
            fflush(stdout);

            if (sleep(1) == -1) 
            {
                perror("sleep");
            }
        }

        ret = pthread_spin_unlock(&spin);
        if (ret != 0) 
        {
            fprintf(stderr, "pthread_spin_unlock (proc2): %s\n", strerror(ret));
            pthread_exit(NULL);
        }

        if (sleep(1) == -1) {

            perror("sleep");
        }
    }

    return NULL;
}


int main()
{
    pthread_t id1, id2;
    int ret;

    ret = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_spin_init: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret = pthread_create(&id1, NULL, proc1, NULL);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_create id1: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret = pthread_create(&id2, NULL, proc2, NULL);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_create id2: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    if (getchar() == EOF) 
    {
        perror("getchar");
    }

    puts("\nКлавиша нажата.");

    flag1 = 1;
    flag2 = 1;

    pthread_join(id1, NULL);
    pthread_join(id2, NULL);

    ret = pthread_spin_destroy(&spin);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_spin_destroy: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    return 0;
}