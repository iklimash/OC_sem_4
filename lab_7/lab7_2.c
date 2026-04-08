#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define FIFO_NAME "/tmp/my_named_pipe"

volatile sig_atomic_t keep_running = 1;
int fifo_fd = -1;

// Поток чтения данных
void* read_thread_func(void* arg) 
{
    char buffer[256];

    while (keep_running) 
    {
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) 
        {
            printf("[Reader] Получено: %s\n", buffer);
        } 
        else if (bytes_read == 0) 
        {
            // Писатель закрыл канал или еще не подключился
            // Выводим сообщение не слишком часто, чтобы не засорять экран
            static int disconnected_msg_shown = 0;
            if (!disconnected_msg_shown) {
                printf("[Reader] Писатель отсоединился (или еще не запущен). Ожидание...\n");
                disconnected_msg_shown = 1;
            }
            sleep(1);
        } 
        else 
        {
            // Ошибка чтения или отсутствие данных в неблокирующем режиме
            if (errno == EAGAIN) {
                // Ресурс временно недоступен (канал пуст)
                // Сбрасываем флаг сообщения, так как писатель подключен
                extern int disconnected_msg_shown; 
                // (хак для сброса статической переменной в рамках примера)
            } else {
                perror("Ошибка read()");
            }
            sleep(1);
        }
    }
    return NULL;
}

int main() {
    pthread_t read_tid;

    // Создаем FIFO
    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) 
    {
        perror("Ошибка mkfifo");
        return 1;
    }

    // Открываем на чтение без блокировки
    fifo_fd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) 
    {
        perror("Ошибка при открытии fifo на чтение");
        return 1;
    }
    
    printf("[Reader] Канал открыт. Ожидание данных...\n");

    // Запускаем поток чтения
    if (pthread_create(&read_tid, NULL, read_thread_func, NULL) != 0)
     {
        perror("Ошибка создания потока чтения");
        return 1;
    }

    printf("Нажмите <Enter> для завершения программы-читателя.\n");
    getchar();

    // Устанавливаем флаг завершения
    keep_running = 0;

    // Дожидаемся завершения потока
    pthread_join(read_tid, NULL);

    // Закрываем дескриптор и удаляем FIFO
    close(fifo_fd);
    unlink(FIFO_NAME);

    printf("[Reader] Программа корректно завершена.\n");
    return 0;
}