#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int flag1 = 0, flag2 = 0;
pthread_spinlock_t spin;

void* proc1(void* args) 
{
    while (flag1 == 0) 
    {
        // ждём захвата спинлока
        while (pthread_spin_trylock(&spin) != 0) {
            usleep(1000); // короткая пауза 1 мс, чтобы не крутить CPU
        }

        // захватили — выводим блок
        for (int i = 0; i < 10; ++i) 
        {
            printf("%c", '1');
            fflush(stdout);
            sleep(1);
        }

        pthread_spin_unlock(&spin); // освобождаем
        sleep(1);
    }
    return NULL;
}

void* proc2(void* args) 
{
    while (flag2 == 0) 
    {
        while (pthread_spin_trylock(&spin) != 0) 
        {
            usleep(1000);
        }

        for (int j = 0; j < 10; ++j) 
        {
            printf("%c", '2');
            fflush(stdout);
            sleep(1);
        }

        pthread_spin_unlock(&spin);
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t id1, id2;

    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

    pthread_create(&id1, NULL, proc1, NULL);
    pthread_create(&id2, NULL, proc2, NULL);

    getchar();
    puts("\nКлавиша нажата.");

    flag1 = 1;
    flag2 = 1;

    pthread_join(id1, NULL);
    pthread_join(id2, NULL);

    pthread_spin_destroy(&spin);
    return 0;
}