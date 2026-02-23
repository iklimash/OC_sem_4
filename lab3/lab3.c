#define _GNU_SOURCE  
#include <stdio.h>      // printf, perror, puts
#include <stdlib.h>     // getenv, exit
#include <unistd.h>     // pipe, read, write, close, sleep
#include <pthread.h>    // pthread_create, pthread_join
#include <string.h>     // strcmp, strlen, memset
#include <signal.h>     // сигнальные типы (не используются напрямую)
#include <fcntl.h>      // fcntl, O_NONBLOCK
#include <errno.h>      // errno
#include <pwd.h>        // getpwnam, struct passwd


int flag1 = 0, flag2 = 0;

// Дескрипторы pipe:
// pipefd[0] — дескриптор для чтения
// pipefd[1] — дескриптор для записи
int pipefd[2];

// Указатель на имя пользователя (получается из переменной окружения USER)
const char *username;


// Поток 1: получает домашний каталог пользователя
// и записывает его в pipe
void* proc1(void *args)
{

    while (!flag1)
    {
        // Получение структуры passwd по имени пользователя.
        struct passwd *pw = getpwnam(username);

        if (pw == NULL)
        {
            perror("Ошибка getpwnam");
            sleep(1);          // Пауза перед повторной попыткой
            continue;          // Переход к следующей итерации цикла
        }
        
        // pw_dir — домашний каталог пользователя
        const char *home_dir = pw->pw_dir;
    
        // Запись строки в pipe.
        // strlen + 1 — передаём вместе с завершающим '\0'
        if (write(pipefd[1], home_dir, strlen(home_dir) + 1) == -1)
        {
            // Если ошибка не связана с неблокирующим режимом
            if (errno != EAGAIN)
            {
                perror("Ошибка записи");
            }

            // Если pipe заполнен (O_NONBLOCK),
            // write вернёт -1 и errno = EAGAIN
            sleep(1);
        }
    }
    return NULL;
}


// Поток 2: читает данные из pipe и выводит их
void* proc2(void *args)
{
    char buffer[256];  // Буфер для чтения данных

    while (!flag2)
    {
        // Обнуляем буфер перед чтением
        memset(buffer, 0, sizeof(buffer));

        // Чтение данных из pipe
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer));

        if (n > 0)
        {
            // Если успешно прочитали данные — выводим
            printf("Домашний каталог: %s\n", buffer);
        }
        else if (n == -1 && errno != EAGAIN)
        {
            // Если произошла ошибка чтения,
            // и она не связана с неблокирующим режимом
            perror("Ошибка чтения");
        }
        sleep(1);
    }

    return NULL;
}

int main (int args, char* argv[])
{
    pthread_t id1, id2;  // Идентификаторы потоков
    int rv;              // Результат системных вызовов

    // Получаем имя пользователя из переменной окружения USER
    username = getenv("USER");

    // Если программа запущена без аргументов
    if (args == 1)
    {
        // Создаём обычный pipe (блокирующий)
        rv = pipe(pipefd);

        // Переводим оба дескриптора в неблокирующий режим
        // F_SETFL перезаписывает флаги, а не добавляет их
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
    }
    else
    {
        // Режим 1 — обычный pipe (блокирующий)
        if (strcmp(argv[1], "1\0") == 0)
        {
            rv = pipe(pipefd);
        }

        // Режим 2 — pipe2 с флагом O_NONBLOCK
        else if (strcmp(argv[1], "2\0") == 0)
        {
            rv = pipe2(pipefd, O_NONBLOCK);
        }

        // Режим 3 — pipe + ручная установка O_NONBLOCK
        else if (strcmp(argv[1], "3\0") == 0)
        {
            rv = pipe(pipefd);
            fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
            fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
        }
    }

    // Создание потоков:
    // id1 выполняет proc1 (запись в pipe)
    // id2 выполняет proc2 (чтение из pipe)
    pthread_create(&id1, NULL, proc1, NULL);
    pthread_create(&id2, NULL, proc2, NULL);

    // Ожидание нажатия клавиши (блокирует главный поток)
    getchar();

    puts("Клавиша нажата.\r\n");

    // Устанавливаем флаги завершения потоков
    flag1 = 1;
    flag2 = 1;

    // Ожидание завершения потоков
    pthread_join(id1,NULL);
    pthread_join(id2,NULL);

    // Закрываем файловые дескрипторы pipe
    close(pipefd[0]);
    close(pipefd[1]);

    return 0;
}