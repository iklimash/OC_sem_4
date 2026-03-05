#include <unistd.h>     // fork(), execlp(), getpid(), getppid(), sleep()
#include <stdio.h>      // printf(), perror()
#include <sys/wait.h>   // waitpid(), WIFEXITED(), WEXITSTATUS()
#include <stdlib.h>     // exit()

int main(int argc, char *argv[]) {

    // Проверка количества аргументов
    if (argc < 3) {
        printf("Ошибка: введите минимум 2 аргумента\n");
        printf("Пример запуска: ./lab4_2 16 11 2004\n");
        return 1;
    }

    // PID текущего процесса
    printf("ID процесса второй программы: %d\n", getpid());

    // PID родителя
    printf("ID родительского процесса второй программы: %d\n", getppid());

    pid_t pid = fork();

    if (pid == -1) {
        perror("Ошибка создания дочернего процесса");
        return 1;
    }

    // Дочерний процесс
    if (pid == 0) {

        printf("Запуск программы lab4_1\n");

        execlp("lab4_1",   // имя без "./"
               "lab4_1",   // argv[0]
               argv[1],    // аргументы из командной строки
               argv[2],
               argv[3],
               NULL);

        perror("Ошибка запуска программы");
        exit(1);
    }

    // Родительский процесс
    else {

        printf("ID дочернего процесса: %d\n", pid);

        int status;
        pid_t w;

        // ожидание завершения дочернего процесса
        while ((w = waitpid(pid, &status, WNOHANG)) == 0) {

            printf("Ждем дочерний процесс...\n");
            sleep(1);
        }

        if (w == -1) {
            perror("Ошибка waitpid");
        }
        else {

            if (WIFEXITED(status)) {
                printf("Дочерний процесс завершился с кодом %d\n",
                       WEXITSTATUS(status));
            }
        }
    }

    return 0;
}