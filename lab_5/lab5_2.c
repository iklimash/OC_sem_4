#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/select.h>
#include <time.h>


sem_t* sem; // Указатель на именованный семафор POSIX
int fd;     // Дескриптор файла для записи
bool flag; 

void deleteSemaphore(const char* msg) 
{
    perror(msg);            // Выводит описание системной ошибки
    sem_close(sem);         // Закрывает дескриптор семафора в текущем процессе
    sem_unlink("/my_sem");  // Удаляет имя семафора из системы (уничтожает его)
    exit(-1);               // Завершает программу с кодом ошибки
}

int main() 
{
    // 1. Открытие или создание именованного семафора
    // "/my_sem" - имя, O_CREAT - создать если нет, 0644 - права доступа, 1 - начальное значение (свободен)
    sem = sem_open("/my_sem", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) 
    {
        deleteSemaphore("Ошибка создания семафора");
    }

    // 2. Открытие файла для записи в режиме добавления (O_APPEND)
    // Если файла нет, он будет создан (O_CREAT)
    fd = open("file.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd == -1) 
    {
        deleteSemaphore("Ошибка открытия файла");
    }

    flag = true;
    printf("Программа 2 запущена. Для выхода нажмите Enter...\n");

    while (flag)
    {
        // --- КРИТИЧЕСКАЯ СЕКЦИЯ ---
        // Уменьшает значение семафора. Если он 0, процесс засыпает и ждет освобождения.
        sem_wait(sem);

        for (int i = 0; i < 10; ++i) 
        {
            write(fd, "2", 1);   
            printf("2");         
            fflush(stdout);       
            sleep(1);            
        }

        // Увеличивает значение семафора, позволяя другому процессу войти в свою критическую секцию.
        sem_post(sem);
        // --- КОНЕЦ КРИТИЧЕСКОЙ СЕКЦИИ ---

        // Подготовка к проверке ввода пользователя с помощью pselect
        fd_set readfds;           // Набор файловых дескрипторов для мониторинга чтения
        FD_ZERO(&readfds);        // Очистка набора
        FD_SET(STDIN_FILENO, &readfds); // Добавляем стандартный ввод (клавиатуру) в набор

        // Настройка таймаута (заменяет usleep)
        struct timespec timeout;
        timeout.tv_sec = 0;             // Секунды
        timeout.tv_nsec = 10000000;     // Наносекунды (10^7 нс = 10 мс)

        /**
         * Вызов pselect():
         * 1. STDIN_FILENO + 1: диапазон проверяемых дескрипторов.
         * 2. &readfds: следим за готовностью к чтению (нажатие клавиш).
         * 3. NULL: запись не проверяем.
         * 4. NULL: исключительные ситуации не проверяем.
         * 5. &timeout: время ожидания (атомарная замена usleep).
         * 6. NULL: маска сигналов (процесс использует свою текущую маску).
         */
        int res = pselect(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout, NULL);

        // Если pselect вернул > 0, значит в потоке ввода появились данные
        if (res > 0 && FD_ISSET(STDIN_FILENO, &readfds)) 
        {
            flag = false; // Пользователь нажал клавишу — завершаем цикл
        }
    }

    // Закрытие ресурсов перед выходом
    if (close(fd) == -1) 
    {
        deleteSemaphore("Ошибка закрытия файла");
    }

    sem_close(sem);

    printf("\nПрограмма завершена.\n");
    return 0;
}