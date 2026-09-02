#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void child_handler(int sig)
{
    printf("SIGCHLD received: Child has terminated\n");
    wait(NULL);
}

int main()
{
    signal(SIGCHLD, child_handler);

    pid_t pid = fork();

    if (pid == 0)
    {
        printf("Child is running ...\n");
        sleep(2);
        printf("Child is exiting ...\n");
    }
    else if (pid > 0)
    {
        printf("Parent is waiting ...\n");
        sleep(5);
        printf("Parent finished.\n");
    }
    else
    {
        printf("Fork failed!\n");
        return 1;
    }

    return 0;
}
