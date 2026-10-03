#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
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

int main(void) {
    char buf[BUF_SIZE];
    ssize_t bytes;

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
                        exit(EXIT_FAILURE);
		    }

                    {
                        char out[64];
                        int len;
                        ssize_t written;

                        len = format_float(result, out, sizeof(out));
                        out[len++] = '\n';

                        written = 0;
                        while (written < len) {
                            ssize_t w = write(STDOUT_FILENO, out + written, len - written);
                            if (w <= 0)
                                fatal("error: child write failed\n");
                            written += w;
                        }
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

    return 0;
}
