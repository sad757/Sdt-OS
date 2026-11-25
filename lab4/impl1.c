#include <math.h>
#include "contract.h"

float cos_derivative(float a, float dx) {
    return (cosf(a + dx) - cosf(a)) / dx;
}

int prime_count(int a, int b) {
    int count = 0;
    for (int i = a; i <= b; i++) {
        if (i < 2) continue;
        int is_prime = 1;
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) count++;
    }
    return count;
}
