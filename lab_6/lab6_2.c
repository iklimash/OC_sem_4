#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

#define SHM_NAME "/my_shm"
#define SEM_WRITE "/sem_write"
#define SEM_READ "/sem_read"
#define SHM_SIZE 256

sem_t *sem_w, *sem_r;
int shm_fd;
char *addr;

volatile sig_atomic_t keep_running = 1;

void cleanup() {
    if (addr) munmap(addr, SHM_SIZE);
    if (shm_fd > 0) close(shm_fd);
    // Примечание: shm_unlink и sem_unlink обычно делает один процесс (Writer),
    // здесь мы просто закрываем дескрипторы.
    if (sem_w) sem_close(sem_w);
    if (sem_r) sem_close(sem_r);
}

void handle_sigint(int sig) {
    printf("\n[EMERGENCY] Reader: Shutting down via Ctrl+C...\n");
    cleanup();
    _exit(1);
}

void* thread_func(void* arg) {
    char local_buf[SHM_SIZE];
    while (keep_running) {
        // Ждем сигнала от Писателя: "Данные в памяти обновлены"
        // Если системный вызов прерван сигналом, проверяем флаг заново
        if (sem_wait(sem_w) == -1) continue; 
        
        // Проверка флага после пробуждения
        if (!keep_running) break;

        // Копируем данные из разделяемой памяти в локальный буфер
        strncpy(local_buf, addr, SHM_SIZE);
        printf("Reader: Received data -> %s\n", local_buf);

        // Сигнализируем Писателю: "Я всё прочитал, можешь обновлять"
        sem_post(sem_r); 
    }
    printf("Reader: Worker thread finished.\n");
    return NULL;
}

int main() {
    pthread_t tid;

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Подключаемся к существующим объектам (те же имена)
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_w = sem_open(SEM_WRITE, 0);
    sem_r = sem_open(SEM_READ, 0);

    pthread_create(&tid, NULL, thread_func, NULL);

    printf("Reader is running. Press [Enter] for normal shutdown...\n");
    
    // Ожидание ввода от пользователя
    getchar();

    printf("\nReader: Starting shutdown...\n");
    
    // Информируем поток о необходимости завершиться
    keep_running = 0;
    sem_post(sem_w); // "Пингуем" семафор, чтобы поток вышел из ожидания
    
    pthread_join(tid, NULL); // Ждем завершения потока
    cleanup();
    
    printf("Reader: Shutdown complete.\n");
    return 0;
}