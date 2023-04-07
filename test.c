#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_ERR_READ 0
#define PIPE_ERR_WRITE 1
#define STD_ERR 2
#define STD_OUT 1
#define STD_IN 0


int pipe_fd[2];
int pipe_err_fd[2];

int child_pid;

int child_process(char *test)
{
    ssize_t sent = write(pipe_fd[PIPE_WRITE], test, strlen(test));
    close(pipe_fd[PIPE_WRITE]);
    printf("wrote: data size: %ld\n", sent);
    if (dup2(pipe_fd[PIPE_READ], STD_IN) == -1)
    {
        fprintf(stderr, "Error dup2: %s\n", strerror(errno));
    }
    if (dup2(pipe_err_fd[PIPE_ERR_WRITE], STD_ERR) == -1)
    {
        fprintf(stderr, "Error err dup2: %s\n", strerror(errno));
    }
    close(pipe_fd[PIPE_READ]);
    close(pipe_fd[PIPE_ERR_WRITE]);
    printf("Launched Excel process with PID: %d\n", child_pid );
    int flag = execl("../OperatingSystem/jsmn/jsondump", "target", (char *)0x0);
    if (flag == -1)
    {
        fprintf(stderr, "Error executing program: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}