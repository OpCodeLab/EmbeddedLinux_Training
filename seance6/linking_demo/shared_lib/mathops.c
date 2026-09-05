#include "mathops.h"

double compute_add(double a, double b)
{
    return a + b;
}

double compute_mul(double a, double b)
{
    return a * b;
}

long compute_factorial(int n)
{
    long result = 1;
    for (int i = 2; i <= n; i++) result *= i;
    return result;
}
