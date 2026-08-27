#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int fd;
    char message[] = "Hello from Writer";

    fd = open("myfifo", O_WRONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    if (write(fd, message, strlen(message) + 1) == -1)
    {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

    printf("Message Sent\n");

    return 0;
}
