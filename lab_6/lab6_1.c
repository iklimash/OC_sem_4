#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>

// Константы для имен ресурсов в системе
#define SHM_NAME "/my_shm"      // Имя объекта разделяемой памяти
#define SEM_WRITE "/sem_write"  // Семафор, разрешающий чтение (данные записаны)
#define SEM_READ "/sem_read"    // Семафор, разрешающий запись (данные прочитаны)
#define SHM_SIZE 256            // Размер буфера

// Глобальные переменные для доступа из функций очистки
sem_t *sem_w, *sem_r;
int shm_fd;
char *addr;

// Флаг состояния: volatile запрещает оптимизацию компилятором, 
// sig_atomic_t гарантирует атомарность доступа при работе с сигналами.
volatile sig_atomic_t keep_running = 1;

// Функция освобождения ресурсов (общая для штатного и аварийного выхода)
void cleanup() {
    if (addr) munmap(addr, SHM_SIZE);
    if (shm_fd > 0) {
        close(shm_fd);
        shm_unlink(SHM_NAME); // Удаляем объект из системы
    }
    if (sem_w) {
        sem_close(sem_w);
        sem_unlink(SEM_WRITE);
    }
    if (sem_r) {
        sem_close(sem_r);
        sem_unlink(SEM_READ);
    }
}

// Обработчик Ctrl+C (SIGINT)
void handle_sigint(int sig) {
    printf("\n[EMERGENCY] Writer: Shutting down via Ctrl+C...\n");
    cleanup();
    _exit(1); // Немедленный выход без стандартных процедур завершения
}

// Функция рабочего потока
void* thread_func(void* arg) {
    struct passwd *pw;
    while (keep_running) {
        // Имитация полезной работы: получаем данные о пользователе
        pw = getpwnam("root"); 
        if (pw) {
            strncpy(addr, pw->pw_dir, SHM_SIZE);
            printf("Writer: Sent data -> %s\n", addr);
        }

        // Сигнализируем Читателю: "Данные готовы, можно читать"
        sem_post(sem_w); 
        
        // Ждем подтверждения от Читателя: "Я закончил читать, пиши снова"
        // Это блокирующая операция.
        sem_wait(sem_r); 
        
        // Если нас разбудили только для того, чтобы мы завершились (keep_running == 0)
        if (!keep_running) break;
        
        sleep(1); // Пауза для наглядности работы
    }
    printf("Writer: Worker thread finished.\n");
    return NULL;
}

int main() {
    pthread_t tid;
    
    // Настройка обработки Ctrl+C через sigaction
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // 1. Создаем объект разделяемой памяти
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHM_SIZE); // Задаем размер
    
    // 2. Отображаем память в адресное пространство процесса
    addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // 3. Инициализируем именованные семафоры
    sem_w = sem_open(SEM_WRITE, O_CREAT, 0666, 0);
    sem_r = sem_open(SEM_READ, O_CREAT, 0666, 0);

    // 4. Запускаем рабочий поток (joinable по умолчанию)
    pthread_create(&tid, NULL, thread_func, NULL);

    printf("Writer is running. Press [Enter] for normal shutdown...\n");
    
    // 5. Точка ожидания: главный поток "висит" здесь до ввода пользователя
    getchar(); 

    printf("\nWriter: Starting graceful...\n");
    
    // 6. Процедура корректного выхода:
    keep_running = 0;      // Меняем флаг
    sem_post(sem_r);       // Разблокируем поток, если он застрял на sem_wait
    
    pthread_join(tid, NULL); // Дожидаемся, пока поток сам завершит цикл и выйдет
    cleanup();             // Чистим ресурсы
    
    printf("Writer: Shutdown complete.\n");
    return 0;
}