#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct
{
    int flag;
    char sym;
}targs;


void* proc1(void* arg1) 
{
    targs* arg = (targs*) arg1;

    while (arg->flag == 0) {
        printf("%c", arg->sym);
        fflush(stdout);
        sleep(1);
    }
    puts("Первый поток закончил работу.\r\n");
    pthread_exit((void*)111);
}

void* proc2(void* arg2) 
{
    targs* arg = (targs*) arg2;
    
    while (arg->flag == 0) {
        printf("%c", arg->sym);
        fflush(stdout);
        sleep(1);
    }
    puts("Второй поток закончил работу.\r\n");
    pthread_exit((void*)222);
}

int main(void) 
{
    pthread_t id1, id2;
    targs args1, args2;
    int *exitCode1, *exitCode2;
    args1.flag = 0; args1.sym = '1';
    args2.flag = 0; args2.sym = '2';
    puts("Ожидание нажатия клавиши.\r\n");
    pthread_create(&id1, NULL, proc1, &args1);
    pthread_create(&id2, NULL, proc2, &args2);
    
    getchar();
    puts("Клавиша нажата.\r\n");
    args1.flag = 1;
    args2.flag = 1;
    pthread_join(id1, (void**)&exitCode1);
    pthread_join(id2, (void**)&exitCode2);
    printf("Код завершения первого потока: %p\r\n", exitCode1);
    printf("Код завершения второго потока: %p\r\n", exitCode2);
    return 0;
}