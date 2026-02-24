#define _GNU_SOURCE  
#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>    
#include <pthread.h>    
#include <string.h>     // функции работы со строками: memset, strlen, snprintf
#include <fcntl.h>      // управление файлами и флагами: O_NONBLOCK, fcntl
#include <pwd.h>        // работа с информацией о пользователях: struct passwd, getpwnam
#include <errno.h>      

// Глобальные флаги для завершения потоков
int flag1 = 0, flag2 = 0;

// Глобальный дескриптор канала (pipe)
// pipefd[0] — для чтения, pipefd[1] — для записи
int pipefd[2];

/* Первый поток — получение домашнего каталога пользователя и запись в pipe */
void* proc1(void* args) 
{
    char buffer[256]; // буфер для хранения пути домашнего каталога

    while (!flag1) 
    {  

        // Получаем имя пользователя, выполняющего процесс
        char *username = getlogin();
        if (username == NULL) 
        {
            // Если getlogin() вернул NULL — выводим ошибку и ждём 1 секунду
            perror("getlogin");
            sleep(1);
            continue; // продолжаем цикл
        }

        // Получаем структуру passwd по имени пользователя
        struct passwd *pwd = getpwnam(username);
        if (pwd == NULL) {
            perror("getpwnam");
            sleep(1);
            continue;
        }

        // Копируем путь домашнего каталога в буфер
        snprintf(buffer, sizeof(buffer), "%s", pwd->pw_dir);

        // Записываем путь в канал
        ssize_t bytes = write(pipefd[1], buffer, strlen(buffer) + 1);
        // strlen(buffer) + 1 — чтобы записать также завершающий нуль строки

        if (bytes == -1) 
        {
            // Если ошибка записи, но не EAGAIN (неблокирующий режим), выводим perror
            if (errno != EAGAIN)
                perror("write");
        }

        sleep(1); 
    }
    return NULL; 
}

/* Второй поток — чтение данных из pipe и вывод их на экран */
void* proc2(void* args) {
    char buffer[256]; // буфер для чтения данных

    while (!flag2) 
    { 

        memset(buffer, 0, sizeof(buffer)); // очищаем буфер перед чтением

        ssize_t bytes = read(pipefd[0], buffer, sizeof(buffer)); // читаем из канала

        if (bytes > 0) 
        {
            // Если прочитали данные — выводим
            printf("Полученный домашний каталог: %s\n", buffer);
        }
        else if (bytes == -1) 
        {
            // Если ошибка чтения, но не EAGAIN (неблокирующий режим), выводим perror
            if (errno != EAGAIN)
                perror("read");
        }

        sleep(1); 
    }
    return NULL; 
}

/* Главная функция */
int main(int argc, char *argv[]) {

    pthread_t id1, id2; // идентификаторы потоков
    int opt;            // переменная для обработки аргументов
    int mode = 0;       // режим работы канала

    /* Разбор аргументов командной строки через getopt */
    while ((opt = getopt(argc, argv, "m:")) != -1) 
    {
        switch (opt) 
        {
            case 'm':
                // Конвертируем аргумент -m в число
                mode = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -m [1|2|3]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* Инициализация канала в зависимости от выбранного режима */
    switch (mode) 
    {

        case 1:   /* pipe() — блокирующий режим */
            if (pipe(pipefd) == -1) 
            { // создаем канал
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            break;

        case 2:   /* pipe2() — неблокирующий режим */
            if (pipe2(pipefd, O_NONBLOCK) == -1) 
            { // создаемканал с O_NONBLOCK
                perror("pipe2");
                exit(EXIT_FAILURE);
            }
            break;

        case 3:   /* pipe() + fcntl() — неблокирующий режим */
            if (pipe(pipefd) == -1) 
            { // создаем обычный блокирующий канал
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            // Меняем флаги на неблокирующий режим для обоих концов
            fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
            fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
            break;

        default:
            fprintf(stderr, "Необходимо указать режим: -m 1 | 2 | 3\n");
            exit(EXIT_FAILURE);
    }

    // Создаем два потока
    pthread_create(&id1, NULL, proc1, NULL); // поток записи
    pthread_create(&id2, NULL, proc2, NULL); // поток чтения

    // Ожидаем нажатия клавиши для завершения программы
    getchar();
    puts("Клавиша нажата.\n");

    // Устанавливаем флаги завершения для потоков
    flag1 = 1;
    flag2 = 1;

    // Дожидаемся завершения потоков
    pthread_join(id1, NULL);
    pthread_join(id2, NULL);

    // Закрываем каналы
    close(pipefd[0]);
    close(pipefd[1]);

    return 0;
}