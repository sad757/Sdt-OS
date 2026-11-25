#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "contract.h"

typedef float (*cos_deriv_func)(float, float);
typedef int (*prime_count_func)(int, int);

static void *lib_handle = NULL;
static cos_deriv_func cos_deriv = NULL;
static prime_count_func prime_cnt = NULL;
static int current_lib = 1;

static int load_library(const char *path) {
    if (lib_handle) {
        dlclose(lib_handle);
        lib_handle = NULL;
    }
    lib_handle = dlopen(path, RTLD_LAZY);
    if (!lib_handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 0;
    }
    cos_deriv = (cos_deriv_func)dlsym(lib_handle, "cos_derivative");
    prime_cnt = (prime_count_func)dlsym(lib_handle, "prime_count");
    if (!cos_deriv || !prime_cnt) {
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(lib_handle);
        lib_handle = NULL;
        return 0;
    }
    return 1;
}

int main(void) {
    if (!load_library("./lib1.so")) return 1;

    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) {
            break;
        }
        if (line[0] == '1') {
            float a, dx;
            if (sscanf(line, "1 %f %f", &a, &dx) == 2) {
                printf("Result: %f\n", cos_deriv(a, dx));
            }
        } else if (line[0] == '2') {
            int a, b;
            if (sscanf(line, "2 %d %d", &a, &b) == 2) {
                printf("Result: %d\n", prime_cnt(a, b));
            }
        } else if (line[0] == '0') {
            if (current_lib == 1) {
                if (!load_library("./lib2.so")) return 1;
                current_lib = 2;
                printf("Switched to lib2\n");
            } else {
                if (!load_library("./lib1.so")) return 1;
                current_lib = 1;
                printf("Switched to lib1\n");
            }
        }
    }

    if (lib_handle) {
        dlclose(lib_handle);
    }
    return 0;
}
