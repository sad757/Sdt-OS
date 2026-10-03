#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024

static void fatal(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

int main(void) {
    char filename[BUF_SIZE];
    ssize_t n = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (n <= 0)
        fatal("error: failed to read filename\n");

    if (filename[n - 1] == '\n')
        filename[n - 1] = '\0';
    else
        filename[n] = '\0';

    int pipe1[2];
    if (pipe(pipe1) == -1)
        fatal("error: failed to create pipe\n");

    pid_t child = fork();
    if (child == -1)
        fatal("error: failed to fork\n");

    if (child == 0) {
        int fd = open(filename, O_RDONLY);
        if (fd == -1)
            fatal("error: child failed to open file\n");

        if (dup2(fd, STDIN_FILENO) == -1)
            fatal("error: dup2 stdin failed\n");
        close(fd);

        if (dup2(pipe1[1], STDOUT_FILENO) == -1)
            fatal("error: dup2 stdout failed\n");

        close(pipe1[0]);
        close(pipe1[1]);

        char *args[] = { "./child", NULL };
        execv("./child", args);

        fatal("error: execv failed\n");
    }

    close(pipe1[1]);

    char buf[BUF_SIZE];
    ssize_t bytes;
    while ((bytes = read(pipe1[0], buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < bytes) {
            ssize_t w = write(STDOUT_FILENO, buf + written, bytes - written);
            if (w <= 0)
                fatal("error: write to stdout failed\n");
            written += w;
        }
    }
    if (bytes < 0)
        fatal("error: read from pipe failed\n");

    close(pipe1[0]);

    int status;
    if (waitpid(child, &status, 0) == -1)
        fatal("error: waitpid failed\n");

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        fatal("error: child terminated due to division by zero\n");

    return 0;
}
