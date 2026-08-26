#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int main() {
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Type something: ");

    while (1) {
        ch = getchar();

        if (ch == 'n') {
            break;
        }

        printf("\nYou pressed: %c", ch);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf("\nExiting ...\n");
    return 0;
}
