#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

int flag1 = 0, flag2 = 0;

int pipefd[2];
const char *usesrname;

void proc1(void *args)
{
    while (!flag1)
    {
        struct passwd *pw = getpwnam(usesrname);
        if (pw == NULL)
        {
            perror("Ошибка getpwnam");
            sleep(1);
            continue;
        }
        
    const char *home_dir = pw->pw_dir;
    
    if (write(pipefd[1], home_dir, strlen(home_dir) + 1) == -1)
    {
        if (errno != EAGAIN)
        {
            perror("Ошибка записи");
        }
        sleep(1);
    }
    return NULL;
    }
}

void proc2(void *args)
{
    char buffer[256];
    while (!flag2)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer));

        if (n > 0)
        {
            printf("Домашний каталог: %s\n", buffer);
        }
        else if (n == -1 && errno != EAGAIN)
        {
            perror("Ошибка чтения");
        }
        sleep(1);
    }
    return NULL;
}

int main (int args, char* argv[])
{
    pthread_t id1, id2;
    int rv;

    if (args == 1)
    {
        rv = pipe(pipefd);
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
    }
    else
    {
        if (strcmp(argv[1], "1\0") == 0)
        {
            rv = pipe(pipefd);
        }
        else if (trcmp(argv[1], "2\0") == 0)
        {
            rv = pipe2(pipefd, O_NONBLOCK);
        }
        else if (trcmp(argv[1], "3\0") == 0)
        {
            rv = pipe(pipefd);
            fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
            fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
        }
    }

    pthread_create(&id1, NULL, proc1, NULL);
    pthread_create(&id2, NULL, proc2, NULL);
    getchar();
    puts("Клавиша нажата.\r\n");
    flag1 = 1;
    flag2 = 2;
    pthread_join(id1,NULL);
    pthread_join(id2,NULL);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}