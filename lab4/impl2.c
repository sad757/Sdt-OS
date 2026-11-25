#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "contract.h"

float cos_derivative(float a, float dx) {
    return (cosf(a + dx) - cosf(a - dx)) / (2.0f * dx);
}

int prime_count(int a, int b) {
    if (b < 2) return 0;
    if (a < 2) a = 2;
    int size = b + 1;
    char *sieve = (char *)malloc(size);
    if (!sieve) return -1;
    memset(sieve, 1, size);
    sieve[0] = sieve[1] = 0;

    for (int i = 2; i * i <= b; i++) {
        if (sieve[i]) {
            for (int j = i * i; j <= b; j += i) {
                sieve[j] = 0;
            }
        }
    }

    int count = 0;
    for (int i = a; i <= b; i++) {
        if (sieve[i]) count++;
    }
    free(sieve);
    return count;
}
