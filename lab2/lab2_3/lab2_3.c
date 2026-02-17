#define _GNU_SOURCE  
#include <stdio.h>   
#include <unistd.h>  
#include <pthread.h> 
#include <string.h>  // Для работы со строками (strerror)
#include <errno.h>   // Для кодов ошибок (EBUSY, errno)
#include <stdlib.h>  


int flag1 = 0, flag2 = 0;
pthread_spinlock_t spin;  // Спинлок для синхронизации доступа к ресурсам

void* proc1(void* args)
{
    int ret;  // Переменная для хранения кодов возврата

    while (!flag1)  
    {
        // Пытаемся захватить спинлок без блокировки
        while ((ret = pthread_spin_trylock(&spin)) != 0)
        {
            if (ret == EBUSY)  // Если спинлок занят другим потоком
            {
                printf("\nПоток 1: спинлок занят\n");
                usleep(1000000);  // Ждем 1 секунду перед повторной попыткой
            } 
            else  // Если произошла другая ошибка
            {
                fprintf(stderr, "pthread_spin_trylock (proc1): %s\n", strerror(ret));
                pthread_exit(NULL);  // Завершаем поток с ошибкой
            }

            // Дополнительная задержка перед проверкой условия цикла
            if (usleep(100000) == -1)  // 0.1 секунды
            {
                perror("usleep");  // Выводим ошибку usleep
            }
        }

        // вывод десяти символов '1'
        for (int i = 0; i < 10; ++i)
        {
            printf("1");           // Печатаем символ
            fflush(stdout);        // Принудительно сбрасываем буфер вывода

            if (sleep(1) == -1)    // Пауза 1 секунда между символами
            {
                perror("sleep");   // Выводим ошибку sleep
            }
        }

        // Освобождаем спинлок
        ret = pthread_spin_unlock(&spin);
        if (ret != 0)  // Если ошибка при разблокировке
        {
            fprintf(stderr, "pthread_spin_unlock (proc1): %s\n", strerror(ret));
            pthread_exit(NULL);
        }

        // Пауза между циклами вывода
        if (sleep(1) == -1) 
        {
            perror("sleep");
        }
    }

    return NULL;  
}

void* proc2(void* args)
{
    int ret;  // Переменная для хранения кодов возврата

    while (!flag2)  
    {
        // Пытаемся захватить спинлок без блокировки
        while ((ret = pthread_spin_trylock(&spin)) != 0)
        {
            if (ret == EBUSY)  // Если спинлок занят
            {
                printf("\nПоток 2: спинлок занят\n");
                usleep(1000000);  // Ждем 1 секунду
            } 
            else  // Другая ошибка
            {
                fprintf(stderr, "pthread_spin_trylock (proc2): %s\n",strerror(ret));
                pthread_exit(NULL);
            }

            // Дополнительная задержка
            if (usleep(100000) == -1)  // 0.1 секунды
            {
                perror("usleep");
            }
        }

        // вывод десяти символов '2'
        for (int i = 0; i < 10; ++i)
        {
            printf("2");           // Печатаем символ
            fflush(stdout);        // Сбрасываем буфер

            if (sleep(1) == -1)    // Пауза 1 секунда
            {
                perror("sleep");
            }
        }

        // Освобождаем спинлок
        ret = pthread_spin_unlock(&spin);
        if (ret != 0) 
        {
            fprintf(stderr, "pthread_spin_unlock (proc2): %s\n",strerror(ret));
            pthread_exit(NULL);
        }

        // Пауза между циклами
        if (sleep(1) == -1) 
        {
            perror("sleep");
        }
    }

    return NULL;  
}

int main()
{
    pthread_t id1, id2;  // Идентификаторы потоков
    int ret;              // Переменная для кодов возврата

    // Инициализация спинлока
    // PTHREAD_PROCESS_PRIVATE - спинлок используется только внутри процесса
    ret = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_spin_init: %s\n", strerror(ret));
        exit(EXIT_FAILURE);  // Завершаем программу с ошибкой
    }

    // Создание первого потока
    ret = pthread_create(&id1, NULL, proc1, NULL);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_create id1: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    // Создание второго потока
    ret = pthread_create(&id2, NULL, proc2, NULL);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_create id2: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    // Ожидание нажатия клавиши пользователем
    if (getchar() == EOF)  // EOF может означать ошибку чтения
    {
        perror("getchar");  // Выводим ошибку, если getchar вернул EOF
    }

    puts("\nКлавиша нажата.");  // Сообщение о нажатии клавиши

    // Устанавливаем флаги для завершения потоков
    flag1 = 1;
    flag2 = 1;

    // Ожидание завершения потоков
    pthread_join(id1, NULL);  // Блокируется пока не завершится поток 1
    pthread_join(id2, NULL);  // Блокируется пока не завершится поток 2

    // Уничтожение спинлока (освобождение ресурсов)
    ret = pthread_spin_destroy(&spin);
    if (ret != 0) 
    {
        fprintf(stderr, "pthread_spin_destroy: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    return 0;  // Успешное завершение программы
}