#define _GNU_SOURCE                 // включение расширений GNU для POSIX
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int flag1 = 0, flag2 = 0;           // флаги для управления завершением потоков
pthread_spinlock_t spin;            // объявление спинлока

// Функция потока 1
void* proc1(void* args) 
{
    while (flag1 == 0)              // пока флаг завершения не установлен
    {
        pthread_spin_lock(&spin);   // захват спинлока для синхронизации
        for (int i = 0; i < 10; ++i) 
        {
            printf("%c", '1');      // вывод символа '1'
            fflush(stdout);          // немедленный вывод на экран
            sleep(1);                // задержка 1 секунда
        }
        pthread_spin_unlock(&spin); // освобождение спинлока
        sleep(1);                    // дополнительная пауза перед следующей итерацией
    }
    pthread_exit(NULL);             // завершение потока
}

// Функция потока 2
void* proc2(void* args) 
{
    while (flag2 == 0)              // пока флаг завершения не установлен
    {
        pthread_spin_lock(&spin);   // захват спинлока для синхронизации
        for (int j = 0; j < 10; ++j) 
        {
            printf("%c", '2');      // вывод символа '2'
            fflush(stdout);          // немедленный вывод на экран
            sleep(1);                // задержка 1 секунда
        }
        pthread_spin_unlock(&spin); // освобождение спинлока
        sleep(1);                    // дополнительная пауза перед следующей итерацией
    }
    pthread_exit(NULL);             // завершение потока
}

int main() {
    pthread_t id1, id2;             // идентификаторы потоков

    // инициализация спинлока
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

    pthread_create(&id1, NULL, proc1, NULL); // создание первого потока
    pthread_create(&id2, NULL, proc2, NULL); // создание второго потока

    getchar();                       // ожидание нажатия клавиши пользователем
    puts("Клавиша нажата.\r\n");

    flag1 = 1;                        // установка флага завершения потока 1
    flag2 = 1;                        // установка флага завершения потока 2

    pthread_join(id1, NULL);           // ожидание завершения потока 1
    pthread_join(id2, NULL);           // ожидание завершения потока 2

    // уничтожение спинлока
    pthread_spin_destroy(&spin);

    return 0;
}