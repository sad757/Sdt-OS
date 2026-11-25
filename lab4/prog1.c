#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contract.h"

int main(void) {
    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        if (line[0] == '1') {
            float a, dx;
            if (sscanf(line, "1 %f %f", &a, &dx) == 2) {
                printf("Result: %f\n", cos_derivative(a, dx));
            }
        } else if (line[0] == '2') {
            int a, b;
            if (sscanf(line, "2 %d %d", &a, &b) == 2) {
                printf("Result: %d\n", prime_count(a, b));
            }
        } else if (line[0] == '0') {
            printf("Program 1: no switch available (linked at compile time)\n");
        } if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) {
   	    break;
	}
    }
    return 0;
}
