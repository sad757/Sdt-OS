#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
#define MAP_SIZE 4096
#define MAX_NUMBERS 64

static void fatal(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

static int format_float(float value, char *out, int out_size) {
    int pos = 0;
    int int_part;
    float frac;
    char tmp[32];
    int t;
    int k;
    int digit;

    if (value < 0.0f) {
        if (pos >= out_size - 1) return pos;
        out[pos++] = '-';
        value = -value;
    }

    int_part = (int)value;

    t = 0;
    if (int_part == 0) {
        tmp[t++] = '0';
    } else {
        while (int_part > 0 && t < 31) {
            tmp[t++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }

    for (k = t - 1; k >= 0 && pos < out_size - 1; k--)
        out[pos++] = tmp[k];

    if (pos < out_size - 1)
        out[pos++] = '.';

    frac = value - (float)((int)value);
    for (k = 0; k < 4 && pos < out_size - 1; k++) {
        frac *= 10.0f;
        digit = (int)frac;
        out[pos++] = '0' + digit;
        frac -= (float)digit;
    }

    out[pos] = '\0';
    return pos;
}

static int str_to_fd(const char *s) {
    int fd = 0;
    int i = 0;

    while (s[i] != '\0') {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        fd = fd * 10 + (s[i] - '0');
        i++;
    }
    return fd;
}

int main(int argc, char **argv) {
    char buf[BUF_SIZE];
    ssize_t bytes;
    int map_fd;
    char *map_ptr;
    int out_pos = 0;

    if (argc < 2)
        fatal("error: missing fd argument\n");

    map_fd = str_to_fd(argv[1]);
    if (map_fd < 0)
        fatal("error: invalid fd argument\n");

    map_ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
    if (map_ptr == MAP_FAILED)
        fatal("error: mmap in child failed\n");

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
        char *cursor;
        char *end;

        buf[bytes] = '\0';
        cursor = buf;
        end = buf + bytes;

        while (cursor < end) {
            char *line_end;
            char *line_start;
            int line_len;
            char saved;

            line_end = cursor;
            while (line_end < end && *line_end != '\n')
                line_end++;

            if (line_end < end) {
                saved = *line_end;
                *line_end = '\0';
            } else {
                saved = '\0';
            }

            line_start = cursor;
            line_len = (int)(line_end - line_start);

            if (line_len > 0) {
                char *saveptr;
                float numbers[MAX_NUMBERS];
                int count;

                saveptr = line_start;
                count = 0;

                while (count < MAX_NUMBERS) {
                    char *next;
                    float val;

                    next = saveptr;
                    val = strtof(saveptr, &next);
                    if (next == saveptr)
                        break;

                    numbers[count++] = val;
                    saveptr = next;
                }

                if (count > 0) {
                    float result;
                    int error;

                    result = numbers[0];
                    error = 0;

                    for (int k = 1; k < count; k++) {
                        if (numbers[k] == 0.0f) {
                            error = 1;
                            break;
                        }
                        result /= numbers[k];
                    }

                    if (error) {
                        map_ptr[out_pos] = '\0';
                        msync(map_ptr, MAP_SIZE, MS_SYNC);
                        munmap(map_ptr, MAP_SIZE);
                        exit(EXIT_FAILURE);
                    }

                    {
                        char out[64];
                        int len;
                        int j;

                        len = format_float(result, out, sizeof(out));
                        out[len++] = '\n';

                        for (j = 0; j < len && out_pos < MAP_SIZE - 1; j++)
                            map_ptr[out_pos++] = out[j];
                    }
                }
            }

            if (line_end < end)
                *line_end = saved;

            cursor = line_end;
            if (cursor < end && *cursor == '\n')
                cursor++;
        }
    }

    if (bytes < 0)
        fatal("error: child read failed\n");

    map_ptr[out_pos] = '\0';
    msync(map_ptr, MAP_SIZE, MS_SYNC);
    munmap(map_ptr, MAP_SIZE);

    return 0;
}
