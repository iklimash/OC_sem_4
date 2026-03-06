#include <unistd.h>     // Библиотека POSIX: содержит функции fork(), execvp(), getpid(), getppid(), sleep()
#include <stdio.h>      // Стандартная библиотека ввода-вывода: printf(), perror()
#include <sys/wait.h>   // Библиотека для работы с ожиданием процессов: waitpid(), макросы WIFEXITED и WEXITSTATUS
#include <stdlib.h>     // Общая стандартная библиотека: getenv(), setenv(), exit()

int main(int argc, char *argv[])
{

    printf("ID процесса второй программы: %d\n", getpid());

    printf("ID родительского процесса второй программы: %d\n", getppid());

    char *path = getenv("PATH");
    // Создаем буфер для нового значения PATH
    char new_path[1024];
    snprintf(new_path, sizeof(new_path), "%s:.", path);
    setenv("PATH", new_path, 1);

    // Создаем дочерний процесс
    pid_t pid = fork();

    // Если fork() вернул -1, произошла ошибка создания процесса
    if (pid == -1)
    {
        perror("Ошибка создания дочернего процесса");
        return 1;
    }

    // Если pid == 0 — мы находимся в дочернем процессе
    if (pid == 0)
    {


        printf("Запуск программы lab4_1\n");

        // Меняем имя программы в argv[0]
        // argv используется как аргументы для новой программы
        argv[0] = "lab4_1";

        // после успешного execvp текущий код больше не выполняется
        execvp("lab4_1", argv);

        perror("Ошибка запуска программы");

        exit(1);
    }
    else
    {

        // Этот код выполняется в родительском процессе
        printf("ID дочернего процесса: %d\n", pid);

        // Переменная для хранения статуса завершения процесса
        int status;

        // Переменная для хранения результата waitpid
        pid_t w;

        // Цикл ожидания завершения дочернего процесса
        // WNOHANG означает:
        // waitpid не блокирует выполнение родителя
        // если ребенок еще работает, возвращается 0
        while ((w = waitpid(pid, &status, WNOHANG)) == 0)
        {
            printf("Ждем дочерний процесс...\n");
            sleep(1);
        }

        if (w == -1)
        {
            perror("Ошибка waitpid");
        }

        // Проверяем, завершился ли процесс нормально
        else if (WIFEXITED(status))
        {
            // Получаем код завершения дочернего процесса
            printf("Дочерний процесс завершился с кодом %d\n", WEXITSTATUS(status));
        }
    }

    return 0;
}