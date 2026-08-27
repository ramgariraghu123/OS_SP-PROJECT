#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    char buffer[100];

    fd = open("myfifo", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    int n = read(fd, buffer, sizeof(buffer) - 1);

    if (n == -1)
    {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';

    printf("Received: %s\n", buffer);

    close(fd);

    return 0;
}

