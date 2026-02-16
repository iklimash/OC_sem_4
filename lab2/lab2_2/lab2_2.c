#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int flag1 = 0, flag2 = 0;
pthread_spinlock_t spin;   // спинлок

void* proc1(void* args) 
{
    while (flag1 == 0) 
    {
        for (int i = 0; i < 10; ++i) 
        {

            pthread_spin_lock(&spin);   // захват

            printf("%c", '1');
            fflush(stdout);

            pthread_spin_unlock(&spin); // освобождение

            sleep(1);
        }
        sleep(1);
    }
    pthread_exit(NULL);
}

void* proc2(void* args) 
{
    while (flag2 == 0) 
    {
        for (int j = 0; j < 10; ++j) 
        {

            pthread_spin_lock(&spin);   // захват

            printf("%c", '2');
            fflush(stdout);

            pthread_spin_unlock(&spin); // освобождение

            sleep(1);
        }
        sleep(1);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t id1, id2;

    // инициализация спинлока
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

    pthread_create(&id1, NULL, proc1, NULL);
    pthread_create(&id2, NULL, proc2, NULL);

    getchar();
    puts("Клавиша нажата.\r\n");

    flag1 = 1;
    flag2 = 1;

    pthread_join(id1, NULL);
    pthread_join(id2, NULL);

    // уничтожение спинлока
    pthread_spin_destroy(&spin);

    return 0;
}