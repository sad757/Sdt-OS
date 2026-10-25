#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>

typedef struct {
    int **matrix;
    int n;
    uint64_t start_perm;
    uint64_t end_perm;
    double partial_result;
} worker_args_t;

static uint64_t factorial(int n) {
    uint64_t r = 1;
    for (int i = 2; i <= n; i++) {
        r *= (uint64_t)i;
        if (r == 0) return 0;
    }
    return r;
}

static int **alloc_matrix(int n) {
    int **m = (int **)malloc((size_t)n * sizeof(int *));
    if (!m) return NULL;
    for (int i = 0; i < n; i++) {
        m[i] = (int *)malloc((size_t)n * sizeof(int));
        if (!m[i]) {
            for (int j = 0; j < i; j++) free(m[j]);
            free(m);
            return NULL;
        }
    }
    return m;
}

static void free_matrix(int **m, int n) {
    if (!m) return;
    for (int i = 0; i < n; i++) free(m[i]);
    free(m);
}

static void fill_matrix(int **m, int n, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            m[i][j] = (rand() % 19) - 9;
}

static void print_matrix(int **m, int n) {
    printf("Matrix %dx%d:\n", n, n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            printf("%4d ", m[i][j]);
        printf("\n");
    }
    printf("\n");
}

static void permutation_rank(uint64_t k, int n, int *perm) {
    int used[16] = {0};
    uint64_t fact[16];
    fact[0] = 1;
    for (int i = 1; i <= n; i++) fact[i] = fact[i-1] * (uint64_t)i;

    for (int i = 0; i < n; i++) {
        uint64_t block = fact[n - 1 - i];
        uint64_t idx = (block > 0) ? (k / block) : 0;
        k = (block > 0) ? (k % block) : 0;

        int count = 0;
        int picked = -1;
        for (int v = 0; v < n; v++) {
            if (!used[v]) {
                if ((uint64_t)count == idx) { picked = v; break; }
                count++;
            }
        }
        perm[i] = picked;
        used[picked] = 1;
    }
}

static int permutation_sign(const int *perm, int n) {
    int inv = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (perm[i] > perm[j]) inv++;
    return (inv % 2 == 0) ? 1 : -1;
}

static double det_range(int **m, int n, uint64_t start, uint64_t end) {
    double sum = 0.0;
    int *perm = (int *)malloc((size_t)n * sizeof(int));
    if (!perm) return 0.0;

    for (uint64_t k = start; k < end; k++) {
        permutation_rank(k, n, perm);
        double prod = 1.0;
        for (int i = 0; i < n; i++) prod *= (double)m[i][perm[i]];
        sum += (double)permutation_sign(perm, n) * prod;
    }
    free(perm);
    return sum;
}

static void *worker(void *arg) {
    worker_args_t *wa = (worker_args_t *)arg;
    wa->partial_result = det_range(wa->matrix, wa->n, wa->start_perm, wa->end_perm);
    return NULL;
}

static double det_parallel(int **m, int n, int T) {
    uint64_t total = factorial(n);
    if (total == 0) return 0.0;

    if ((uint64_t)T > total) T = (int)total;
    if (T <= 0) T = 1;

    pthread_t *threads = (pthread_t *)malloc((size_t)T * sizeof(pthread_t));
    worker_args_t *args = (worker_args_t *)malloc((size_t)T * sizeof(worker_args_t));
    if (!threads || !args) {
        free(threads);
        free(args);
        return 0.0;
    }

    uint64_t base = total / (uint64_t)T;
    uint64_t rem = total % (uint64_t)T;
    uint64_t cursor = 0;

    for (int i = 0; i < T; i++) {
        uint64_t chunk = base + ((uint64_t)i < rem ? 1 : 0);
        args[i].matrix = m;
        args[i].n = n;
        args[i].start_perm = cursor;
        args[i].end_perm = cursor + chunk;
        args[i].partial_result = 0.0;
        cursor += chunk;
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    double total_result = 0.0;
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
        total_result += args[i].partial_result;
    }

    free(threads);
    free(args);
    return total_result;
}

static double time_now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void print_usage(const char *prog) {
    printf("Usage: %s -n <N> [-t <T>] [-s <seed>] [-v] [--bench] [--csv]\n", prog);
    printf("  -n N      Matrix size N x N (max 12)\n");
    printf("  -t T      Number of threads (default: 1)\n");
    printf("  -s S      Seed for random (default: 42)\n");
    printf("  -v        Show generated matrix\n");
    printf("  --bench   Run full benchmark (T=1,2,4,8,16,128,1024)\n");
    printf("  --csv     Output CSV format (with --bench)\n");
}

int main(int argc, char **argv) {
    int n = 0;
    int T = 1;
    unsigned int seed = 42;
    int verbose = 0;
    int bench = 0;
    int csv = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) { n = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { T = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { seed = (unsigned int)atoi(argv[++i]); }
        else if (strcmp(argv[i], "-v") == 0) { verbose = 1; }
        else if (strcmp(argv[i], "--bench") == 0) { bench = 1; }
        else if (strcmp(argv[i], "--csv") == 0) { csv = 1; }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) { print_usage(argv[0]); return 0; }
        else { print_usage(argv[0]); return 1; }
    }

    if (n <= 0 || n > 12) {
        fprintf(stderr, "Error: -n must be between 1 and 12\n");
        return 1;
    }

    int **matrix = alloc_matrix(n);
    if (!matrix) { fprintf(stderr, "Error: alloc failed\n"); return 1; }
    fill_matrix(matrix, n, seed);

    if (verbose) print_matrix(matrix, n);

    if (!bench) {
        double t0 = time_now_seconds();
        double det_seq = det_range(matrix, n, 0, factorial(n));
        double t1 = time_now_seconds();
        double seq_ms = (t1 - t0) * 1000.0;

        t0 = time_now_seconds();
        double det_par = det_parallel(matrix, n, T);
        t1 = time_now_seconds();
        double par_ms = (t1 - t0) * 1000.0;

        printf("n=%d seed=%u\n", n, seed);
        printf("Sequential:  det = %.0f  time = %.2f ms\n", det_seq, seq_ms);
        printf("Parallel T=%d: det = %.0f  time = %.2f ms\n", T, det_par, par_ms);
        if (seq_ms > 0) {
            double speedup = seq_ms / par_ms;
            double eff = speedup / (double)T;
            printf("Speedup = %.2f, Efficiency = %.2f\n", speedup, eff);
        }
        if (fabs(det_seq - det_par) > 1e-3) {
            fprintf(stderr, "WARNING: results differ! seq=%.0f par=%.0f\n", det_seq, det_par);
        }
    } else {
        int thread_counts[] = {1, 2, 4, 6, 8, 16, 128, 1024};
        int count = (int)(sizeof(thread_counts) / sizeof(thread_counts[0]));

        det_range(matrix, n, 0, factorial(n));

        double seq_time = 0.0;
        {
            double t0 = time_now_seconds();
            det_range(matrix, n, 0, factorial(n));
            double t1 = time_now_seconds();
            seq_time = (t1 - t0) * 1000.0;
        }

        double t1_ref = 0.0;
        {
            double t0 = time_now_seconds();
            det_parallel(matrix, n, 1);
            double t1 = time_now_seconds();
            t1_ref = (t1 - t0) * 1000.0;
        }

        if (!csv) {
            printf("n=%d, seed=%u\n", n, seed);
            printf("Sequential time: %.2f ms\n", seq_time);
            printf("T=1 reference time: %.2f ms\n\n", t1_ref);
            printf("T    | Time (ms) | Speedup | Efficiency\n");
            printf("-----|-----------|---------|------------\n");
        } else {
            printf("threads,time_ms,speedup,efficiency\n");
        }

        for (int i = 0; i < count; i++) {
            int t = thread_counts[i];
            double t0 = time_now_seconds();
            double det = det_parallel(matrix, n, t);
            (void)det;
            double t1 = time_now_seconds();
            double ms = (t1 - t0) * 1000.0;
            double sp = (ms > 0 && t1_ref > 0) ? (t1_ref / ms) : 0.0;
            double ef = (t > 0) ? (sp / (double)t) : 0.0;
	    if (t == 1) { sp = 1.0; ef = 1.0; }
            if (csv) printf("%d,%.2f,%.2f,%.4f\n", t, ms, sp, ef);
            else     printf("%-4d | %-9.2f | %-7.2f | %.4f\n", t, ms, sp, ef);
        }
    }

    free_matrix(matrix, n);
    return 0;
}
