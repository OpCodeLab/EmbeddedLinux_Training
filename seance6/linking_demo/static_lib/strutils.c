#include <stdio.h>
#include <string.h>
#include "strutils.h"

void print_banner(const char *title)
{
    size_t len = strlen(title);

    for (size_t i = 0; i < len + 4; i++) putchar('=');
    putchar('\n');

    printf("| %s |\n", title);

    for (size_t i = 0; i < len + 4; i++) putchar('=');
    putchar('\n');
}
