#define _GNU_SOURCE             /* требуется для gettid() */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>        /* для syscall() */
#include <sys/types.h>

typedef struct
{
    int flag;           /* флаг завершения работы потока */
    char sym;           /* символ, выводимый потоком */
    pthread_t tid;      /* идентификатор pthread_self() */
    pid_t linux_tid;    /* идентификатор нити (gettid) */
} targs;

/* Поточная функция №1 */
void* proc1(void* arg1) 
{
    printf("поток 1 начал работу\n");

    targs* arg = (targs*) arg1; /* приведение универсального указателя к типу targs */

    arg->tid = pthread_self();  /* получение идентификатора POSIX-потока */
    arg->linux_tid = syscall(SYS_gettid); /* получение идентификатора нити ядра */

    printf("поток 1: pthread_self() = %lu\n", (unsigned long)arg->tid);
    printf("поток 1: gettid() = %d\n", arg->linux_tid);
    printf("поток 1: getpid() = %d\n", getpid()); /* получение PID процесса */

    while (arg->flag == 0)
    {
        printf("%c", arg->sym);
        fflush(stdout); /* принудительная очистка буфера stdout */
        sleep(1);       /* приостановка выполнения на 1 секунду */
    }

    printf("\nпоток 1 закончил работу\n");

    pthread_exit((void*)111); /* завершение потока */
}

/* Поточная функция №2 */
void* proc2(void* arg2) 
{
    printf("поток 2 начал работу\n");

    targs* arg = (targs*) arg2;

    arg->tid = pthread_self();  /* получение идентификатора POSIX-потока */
    arg->linux_tid = syscall(SYS_gettid); /* получение идентификатора нити ядра */

    printf("поток 2: pthread_self() = %lu\n", (unsigned long)arg->tid);
    printf("поток 2: gettid() = %d\n", arg->linux_tid);
    printf("поток 2: getpid() = %d\n", getpid()); /* получение PID процесса */

    while (arg->flag == 0)
    {
        printf("%c", arg->sym);
        fflush(stdout);
        sleep(1);
    }

    printf("\nпоток 2 закончил работу\n");

    pthread_exit((void*)222);
}

int main(void) 
{
    printf("программа начала работу\n");

    pthread_t id1, id2; /* идентификаторы потоков, полученные pthread_create */
    targs args1, args2;

    void* exitCode1;
    void* exitCode2;

    args1.flag = 0; 
    args1.sym = '1';

    args2.flag = 0; 
    args2.sym = '2';

    printf("main: getpid() = %d\n", getpid()); /* получение идентификатора процесса */
    printf("main: gettid() = %ld\n", syscall(SYS_gettid)); /* идентификатор нити main */

    int rv1 = pthread_create(&id1, NULL, proc1, &args1); 
    /* создание первого потока */

    int rv2 = pthread_create(&id2, NULL, proc2, &args2); 
    /* создание второго потока */

    sleep(2); /* задержка для гарантии инициализации tid в потоках */

    printf("\nСравнение идентификаторов:\n");

    printf("id1 (pthread_create) = %lu\n", (unsigned long)id1);
    printf("tid1 (pthread_self)  = %lu\n", (unsigned long)args1.tid);

    printf("id2 (pthread_create) = %lu\n", (unsigned long)id2);
    printf("tid2 (pthread_self)  = %lu\n", (unsigned long)args2.tid);

    /* сравнение id1 и tid1 */
    if (pthread_equal(id1, args1.tid))
        printf("id1 и pthread_self() потока 1 совпадают\n");
    else
        printf("id1 и pthread_self() потока 1 НЕ совпадают\n");

    /* сравнение id2 и tid2 */
    if (pthread_equal(id2, args2.tid))
        printf("id2 и pthread_self() потока 2 совпадают\n");
    else
        printf("id2 и pthread_self() потока 2 НЕ совпадают\n");

    /* сравнение идентификаторов разных потоков */
    if (pthread_equal(id1, id2))
        printf("id1 и id2 совпадают (ошибка)\n");
    else
        printf("id1 и id2 различны (корректно)\n");

    printf("программа ждет нажатия клавиши\n");

    getchar(); /* ожидание ввода */

    printf("клавиша нажата\n");

    args1.flag = 1;
    args2.flag = 1;

    pthread_join(id1, &exitCode1); /* ожидание завершения потока 1 */
    pthread_join(id2, &exitCode2); /* ожидание завершения потока 2 */

    printf("код завершения первого потока: %ld\n", (long)exitCode1);
    printf("код завершения второго потока: %ld\n", (long)exitCode2);
}