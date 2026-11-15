#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
#define MAP_SIZE 4096

static void fatal(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

static int fd_to_str(int fd, char *out) {
    char rev[16];
    int i = 0;
    int j = 0;

    if (fd == 0) {
        out[0] = '0';
        out[1] = '\0';
        return 2;
    }
    while (fd > 0) {
        rev[i++] = '0' + (fd % 10);
        fd /= 10;
    }
    while (i > 0)
        out[j++] = rev[--i];
    out[j] = '\0';
    return j + 1;
}

int main(void) {
    char filename[BUF_SIZE];
    ssize_t n;
    int map_fd;
    char *map_ptr;
    pid_t child;
    int status;
    char fd_str[16];

    n = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (n <= 0)
        fatal("error: failed to read filename\n");

    if (filename[n - 1] == '\n')
        filename[n - 1] = '\0';
    else
        filename[n] = '\0';

    map_fd = open("mmap_file", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (map_fd == -1)
        fatal("error: failed to open mmap_file\n");

    if (ftruncate(map_fd, MAP_SIZE) == -1)
        fatal("error: ftruncate failed\n");

    map_ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
    if (map_ptr == MAP_FAILED)
        fatal("error: mmap failed\n");

    fd_to_str(map_fd, fd_str);

    child = fork();
    if (child == -1)
        fatal("error: failed to fork\n");

    if (child == 0) {
        int fd;
        char *args[3];

        fd = open(filename, O_RDONLY);
        if (fd == -1)
            fatal("error: child failed to open file\n");

        if (dup2(fd, STDIN_FILENO) == -1)
            fatal("error: dup2 stdin failed\n");
        close(fd);

        args[0] = "./child";
        args[1] = fd_str;
        args[2] = NULL;

        execv("./child", args);

        fatal("error: execv failed\n");
    }

    close(map_fd);

    if (waitpid(child, &status, 0) == -1)
        fatal("error: waitpid failed\n");

    {
        ssize_t len = 0;
        ssize_t written = 0;

        while (len < MAP_SIZE && map_ptr[len] != '\0')
            len++;

        while (written < len) {
            ssize_t w = write(STDOUT_FILENO, map_ptr + written, len - written);
            if (w <= 0)
                fatal("error: write to stdout failed\n");
            written += w;
        }
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        munmap(map_ptr, MAP_SIZE);
        unlink("mmap_file");
        fatal("error: child terminated due to division by zero\n");
    }

    munmap(map_ptr, MAP_SIZE);
    unlink("mmap_file");

    return 0;
}
