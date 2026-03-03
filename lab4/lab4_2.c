#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    printf("ID процесса второй программы: %d\n", getpid());
    printf("ID родительского процесса второй программы: %d\n", getppid());

    pid_t pid = fork();

    if (pid == -1) {
        perror("Ошибка создания дочернего процесса");
        return 1;
    }
    else if (pid == 0) {
        // Устанавливаем переменную окружения
        if (setenv("MY_VAR", "Письмо_от_родителя!", 1) == -1) {
            perror("Ошибка установки переменной окружения");
            exit(1);
        }

        // execlp ищет программу в PATH
        execlp("./lab4_1",
               "lab4_1",   // argv[0] — имя программы
               "16",
               "11",
               "2004",
               "26",
               "02",
               NULL);

        // Если execlp вернулся — произошла ошибка
        perror("Ошибка исполнения дочерней программы");
        exit(1);
    }
    else {
        printf("ID дочернего процесса второй программы: %d\n", pid);

        int status;
        pid_t w;

        while ((w = waitpid(pid, &status, WNOHANG)) == 0) {
            printf("Ждем дочерний процесс\n");
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