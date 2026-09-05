#include <stdio.h>
#include "strutils.h"   /* from the STATIC library */
#include "mathops.h"    /* from the SHARED library (.so) */

int main(void)
{
    /* Function from the static lib: its code is baked into this binary */
    print_banner("Static + Shared linking demo");

    /* Functions from the shared lib: resolved at runtime via libmathops.so */
    printf("compute_add(3, 4)       = %.1f\n", compute_add(3, 4));
    printf("compute_mul(3, 4)       = %.1f\n", compute_mul(3, 4));
    printf("compute_factorial(5)    = %ld\n", compute_factorial(5));

    return 0;
}
