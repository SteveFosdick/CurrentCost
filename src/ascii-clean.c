#include <stdio.h>

int main(int argc, char **argv) {
    int ch;

    while ((ch = getchar()) != EOF)
	if ((ch >= 0x20 && ch <= 0x7e) || ch == '\n')
	    putchar(ch);
    return 0;
}
