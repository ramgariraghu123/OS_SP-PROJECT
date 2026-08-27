#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    int fd[2];
    pid_t pid;

    char message[] = "Hello from Parent";
    char buffer[100];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        // Child
        close(fd[1]);

        int n = read(fd[0], buffer, sizeof(buffer) - 1);

        if (n == -1)
        {
            perror("read");
            close(fd[0]);
            return 1;
        }

        buffer[n] = '\0';

        printf("Child Received : %s\n", buffer);

        close(fd[0]);
    }
    else
    {
        // Parent
        close(fd[0]);

        if (write(fd[1], message, strlen(message) + 1) == -1)
        {
            perror("write");
            close(fd[1]);
            wait(NULL);
            return 1;
        }

        close(fd[1]);

        wait(NULL);
    }

    return 0;
}

