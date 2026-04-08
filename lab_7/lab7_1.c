#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>

#define FIFO_NAME "/tmp/my_named_pipe"

volatile sig_atomic_t keep_running_open = 1;
volatile sig_atomic_t keep_running_write = 1;

int fifo_fd = -1;
pthread_t write_tid;
int write_thread_created = 0;

// Обработчик сигнала закрытого канала
void sig_handler(int signo) {
    if (signo == SIGPIPE) {
        printf("[Writer] Получен сигнал SIGPIPE (читатель отсоединился).\n");
    }
}

// Поток передачи данных
void* write_thread_func(void* arg) {
    struct passwd *pw;
    const char *username = "root"; // Имя пользователя для getpwnam

    while (keep_running_write) {
        pw = getpwnam(username);
        if (pw == NULL) {
            perror("Ошибка getpwnam");
            sleep(1);
            continue;
        }

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Домашняя директория '%s': %s", username, pw->pw_dir);

        ssize_t bytes_written = write(fifo_fd, buffer, strlen(buffer) + 1);
        
        if (bytes_written == -1) {
            if (errno == EPIPE) {
                // Сигнал обработан в sig_handler, ждем
            } else if (errno != EAGAIN) {
                perror("Ошибка записи write()");
            }
        } else {
            printf("[Writer] Отправлено: %s\n", buffer);
        }

        sleep(1);
    }
    return NULL;
}

// Поток открытия канала
void* open_thread_func(void* arg) {
    printf("[Writer] Запуск потока. Ожидание подключения читателя...\n");
    
    while (keep_running_open) {
        fifo_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
        
        if (fifo_fd == -1) {
            if (errno == ENXIO) {
                // ВЫВОДИМ ОШИБКУ PERROR ЗДЕСЬ
                perror("[Writer] Ожидание читателя (ENXIO)");
                sleep(1);
            } else {
                perror("Ошибка при открытии fifo");
                sleep(1);
            }
        } else {
            printf("[Writer] Читатель подключился! Канал открыт.\n");
            
            if (pthread_create(&write_tid, NULL, write_thread_func, NULL) == 0) {
                write_thread_created = 1;
            } else {
                perror("Ошибка создания потока записи");
            }
            break; 
        }
    }
    return NULL;
}

int main() {
    pthread_t open_tid;

    signal(SIGPIPE, sig_handler);

    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) {
        perror("Ошибка mkfifo");
        return 1;
    }

    if (pthread_create(&open_tid, NULL, open_thread_func, NULL) != 0) {
        perror("Ошибка создания потока открытия");
        return 1;
    }

    printf("Нажмите <Enter> для завершения программы-писателя.\n");
    getchar();

    keep_running_open = 0;
    keep_running_write = 0;

    pthread_join(open_tid, NULL);
    if (write_thread_created) {
        pthread_join(write_tid, NULL);
    }

    if (fifo_fd != -1) {
        close(fifo_fd);
    }
    unlink(FIFO_NAME);

    printf("[Writer] Программа корректно завершена.\n");
    return 0;
}