#include <stdio.h>

int main(int argc, char **argv)
{
    int ch, count, max;

    for (count = max = 0; (ch = getchar()) != EOF;)
        if (ch == '\n') {
            if (count > max)
                max = count;
            count = 0;
        }
        else
            count++;
    printf("%d\n", max);
    return 0;
}
