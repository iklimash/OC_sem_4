#define _GNU_SOURCE  // включаем расширения GNU, чтобы использовать usleep и другие функции
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>  // для strerror()

int flag1 = 0, flag2 = 0;  // флаги для остановки потоков
pthread_spinlock_t spin;    // объявление спинлока

// Функция для первого потока
void* proc1(void* args) 
{
    int lock_result;
    
    while (flag1 == 0)  // основной цикл, пока флаг не установлен
    {
        // пробуем захватить спинлок, пока не получится
        while ((lock_result = pthread_spin_trylock(&spin)) != 0) {
            // Выводим сообщение об ошибке при любой неудачной попытке захвата
            printf("\nПоток 1: ошибка при попытке захвата спинлока (код: %d)\n", lock_result);
            fflush(stdout);
            usleep(1000000); // ждем 1 с, чтобы не перегружать CPU
        }

        // захватили спинлок — выводим символ '1' 10 раз
        for (int i = 0; i < 10; ++i) 
        {
            printf("%c", '1');   // выводим символ
            fflush(stdout);      // сбрасываем буфер вывода
            sleep(1);            // пауза 1 секунда между символами
        }

        pthread_spin_unlock(&spin); // освобождаем спинлок
        sleep(1); // небольшая пауза перед новой попыткой
    }
    return NULL; // завершение потока
}

// Функция для второго потока (аналогично первому)
void* proc2(void* args) 
{
    int lock_result;
    
    while (flag2 == 0) 
    {
        while ((lock_result = pthread_spin_trylock(&spin)) != 0) 
        {
            // Выводим сообщение об ошибке при любой неудачной попытке захвата
            printf("\nПоток 2: ошибка при попытке захвата спинлока (код: %d)\n", lock_result);
            fflush(stdout);
            usleep(1000000); // ждем 1 с перед повторной попыткой
        }

        // захватили спинлок — выводим символ '2' 10 раз
        for (int j = 0; j < 10; ++j) 
        {
            printf("%c", '2');
            fflush(stdout);
            sleep(1);
        }

        pthread_spin_unlock(&spin); // освобождаем спинлок
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t id1, id2; // идентификаторы потоков
    int init_result;

    init_result = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE); // инициализация спинлока для потоков внутри процесса
    if (init_result != 0) {
        printf("Ошибка инициализации спинлока: %s\n", strerror(init_result));
        return 1;
    }

    if (pthread_create(&id1, NULL, proc1, NULL) != 0) {
        printf("Ошибка создания потока 1\n");
        return 1;
    }
    
    if (pthread_create(&id2, NULL, proc2, NULL) != 0) {
        printf("Ошибка создания потока 2\n");
        return 1;
    }

    getchar(); // ждем нажатия клавиши пользователем
    puts("\nКлавиша нажата."); // сообщение о том, что клавиша нажата

    flag1 = 1; // устанавливаем флаги, чтобы потоки завершились
    flag2 = 1;

    pthread_join(id1, NULL); // ожидание завершения первого потока
    pthread_join(id2, NULL); // ожидание завершения второго потока

    pthread_spin_destroy(&spin); // уничтожение спинлока
    return 0; // завершение программы
}