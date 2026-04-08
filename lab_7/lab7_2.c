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
void* read_thread_func(void* arg) {
    char buffer[256];

    while (keep_running) {
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            printf("[Reader] Получено: %s\n", buffer);
        } else if (bytes_read == 0) {
            // Писатель не подключен. 
            // read == 0 не устанавливает errno, поэтому perror здесь выведет "Success".
            // Заменяем на printf, чтобы было видно, что цикл работает.
            printf("[Reader] Писатель отсоединился (или еще не запущен). Возврат read: 0\n");
            sleep(1);
        } else {
            if (errno == EAGAIN) {
                // Выводим ошибку EAGAIN через perror
                perror("[Reader] Нет данных (EAGAIN)");
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

    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) {
        perror("Ошибка mkfifo");
        return 1;
    }

    fifo_fd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("Ошибка при открытии fifo на чтение");
        return 1;
    }
    
    printf("[Reader] Канал открыт. Ожидание данных...\n");

    if (pthread_create(&read_tid, NULL, read_thread_func, NULL) != 0) {
        perror("Ошибка создания потока чтения");
        return 1;
    }

    printf("Нажмите <Enter> для завершения программы-читателя.\n");
    getchar();

    keep_running = 0;

    pthread_join(read_tid, NULL);

    close(fifo_fd);
    unlink(FIFO_NAME);

    printf("[Reader] Программа корректно завершена.\n");
    return 0;
}