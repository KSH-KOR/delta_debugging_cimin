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

int does_contain_error_msg(char *error_msg);
int child_process();
int parent_process(char *test);
int save_in_file(char *target, char *filename);
int execute_target_program();
char *reduce(char *t);
void handle_sigint(int sig);

int pipe_fd[2];
int pipe_err_fd[2];

char *input_file_path = NULL;
char *output_file_path = NULL;
char *error_string = NULL;
char **program_args;

char *reduced_input = NULL;
char *original_input = NULL;

int child_pid;

int does_contain_error_msg(char *error_msg)
{
    printf("error msg::\n%s\n", error_msg);
    return strstr(error_msg, error_string) != NULL;
}
int save_in_file(char *target, char *filename)
{
    // write reduced input to file
    FILE *output_file = fopen(filename, "w");
    if (output_file == NULL)
    {
        fprintf(stderr, "Error opening output file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fwrite(target, 1, strlen(target), output_file);
    fclose(output_file);
}
int execute_target_program()
{
    int flag;
    if (sizeof(program_args[0]) == sizeof(program_args))
    {
        flag = execl(program_args[0], program_args[0], (char *)0x0);
    }
    else
    {
        flag = execv(program_args[0], &(program_args[1]));
    }
    return flag;
}
int read_input_from_file()
{
    FILE *fp;
    char *buf;
    long file_size;

    // Open the file in binary mode
    fp = fopen(input_file_path, "rb");

    if (fp == NULL)
    {
        printf("Error opening file.\n");
        exit(1);
    }

    // Determine the size of the file
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for the buffer
    buf = (char *)malloc(file_size + 1);
    if (buf == NULL)
    {
        printf("Error allocating memory.\n");
        exit(1);
    }

    // Read the contents of the file into the buffer
    fread(buf, file_size, 1, fp);
    buf[file_size] = '\0'; // Add null terminator to end of buffer

    // Close the file
    fclose(fp);

    // save the contents of the buffer
    original_input = (char *)malloc(file_size + 1);
    strcpy(original_input, buf);

    // Free the memory allocated for the buffer
    free(buf);
    return 0;
}
void handle_sigint(int sig)
{
    if (child_pid > 0)
    {
        kill(child_pid, SIGKILL);
    }
    char buffer[BUFFER_SIZE];
    lseek(pipe_fd[0], 0, SEEK_SET);
    int num_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1);
    buffer[num_read] = '\0';
    printf("Shortest crashing input size: %d\n", num_read);
    printf("Output: %s", buffer);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    close(pipe_err_fd[0]);
    close(pipe_err_fd[1]);
    exit(0);
}

int child_process()
{
    // close(pipe_fd[PIPE_WRITE]);
    // close(pipe_err_fd[PIPE_ERR_READ]);
    FILE *input_fp = fopen(input_file_path, "rb");
    int input_fd = fileno(input_fp);

    if (dup2(pipe_fd[PIPE_READ], STD_IN) == -1 || dup2(pipe_err_fd[PIPE_ERR_WRITE], STD_ERR) == -1)
    {

        fprintf(stderr, "Error dup2: %s\n", strerror(errno));
    }

    close(pipe_fd[PIPE_READ]);
    close(pipe_err_fd[PIPE_ERR_WRITE]);

    int flag = execute_target_program();
    if (flag == -1)
    {
        fprintf(stderr, "Error executing program: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
int timeout = 3;
int parent_process(char *test)
{
    // close(pipe_fd[PIPE_READ]);
    // close(pipe_err_fd[PIPE_ERR_WRITE]);

    printf("Launched Excel process with PID\n");

    // Set up the select call to monitor the pipe for readability
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(pipe_err_fd[PIPE_ERR_READ], &readfds);

    struct timeval timeout_val;
    timeout_val.tv_sec = timeout;
    timeout_val.tv_usec = 0;

    int result = select(pipe_err_fd[PIPE_ERR_READ] + 1, &readfds, NULL, NULL, &timeout_val);
    if (result == -1)
    {
        perror("Failed to wait for pipe");
        exit(EXIT_FAILURE);
    }
    else if (result == 0)
    {
        printf("Timeout waiting for data on the pipe. Sending SIGKILL to Excel process...\n");

        // Forcefully terminate the Excel process with SIGKILL
        if (kill(child_pid, SIGKILL) == 0)
        {
            printf("Sent SIGKILL to Excel process with PID \n");
        }
        else
        {
            perror("Failed to send SIGKILL");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        write(pipe_fd[PIPE_WRITE], test, strlen(test));
        close(pipe_fd[PIPE_WRITE]);

        char buffer[BUFFER_SIZE];

        // read from pipe until error string is found or end of file
        int bytes_read;
        while ((bytes_read = read(pipe_err_fd[PIPE_ERR_READ], buffer, BUFFER_SIZE - 1)) > 0)
        {
            if (does_contain_error_msg(buffer))
            {
                printf("error msg found. kill child proc");
                kill(child_pid, SIGINT);
                return 0;
            }
        }
        fprintf(stderr, "Error string not found in output\n");
    }
    // close(pipe_err_fd[PIPE_ERR_READ]);

    // error string not found

    return -1;
}
int c = 0;
// 𝑝, a target programt that crashes p
// 𝑐, a predicate over output to check if the program crashed
char *reduce(char *t)
{
    char *tm = t;
    int s = strlen(tm) - 1;
    printf("\ncalled : %d\n", ++c);
    while (s > 0)
    {
        for (int i = 0; i < strlen(tm) - s; i++)
        {
            char *head = strndup(tm, i); // The strndup(s, n) function copies at most n bytes. If s is longer than n, only n bytes are copied, and a terminating null byte('\0') is added.
            char *tail = strndup(&tm[i + s], strlen(tm) - 1);
            char *test = malloc(strlen(head) + strlen(tail) + 1);
            strcpy(test, head);
            strcat(test, tail);

            printf("\ntest input: %s\n", test);

            child_pid = fork();
            if (child_pid == 0)
            { // child process
                child_process();
            }
            else
            { // parent process
                if (parent_process(test) == 0)
                {
                    printf("1\n");
                    char *reduced = reduce(test);
                    free(test);
                    free(head);
                    free(tail);
                    return reduced;
                }
            }

            free(test);
            free(head);
            free(tail);
        }

        for (int i = 0; i < strlen(tm) - s; i++)
        {
            char *mid = strndup(&tm[i], s);

                        printf("\ntest input: %s\n", mid);


            child_pid = fork();
            if (child_pid == 0)
            { // child process
                child_process(mid);
            }
            else
            { // parent process
                if (parent_process(mid) == 0)
                {
                    printf("2\n");
                    char *reduced = reduce(mid);
                    free(mid);
                    return reduced;
                }
            }

            free(mid);
        }
        s--;
    }
    return tm;
}

char *minimize(char *t)
{
    return reduce(t);
}

int main(int argc, char *argv[])
{
    // parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "i:m:o:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            input_file_path = optarg;
            break;
        case 'm':
            error_string = optarg;
            break;
        case 'o':
            output_file_path = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -i input_path -m error_string -o output_path\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // check if required arguments are provided
    if (input_file_path == NULL || error_string == NULL || output_file_path == NULL)
    {
        fprintf(stderr, "Usage: %s -i input_path -m error_string -o output_path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    program_args = &argv[optind]; // assume all the option arguments are handled.

    printf("\n--------------------------------\n");
    printf("input_path: %s\n", input_file_path);
    printf("error_string: %s\n", error_string);
    printf("output_path: %s\n", output_file_path);
    printf("target program path: %s\n", program_args[0]);
    printf("--------------------------------\n");
    // create pipe
    if (pipe(pipe_fd) == -1 || pipe(pipe_err_fd) == -1)
    {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("pipe file descriptors: %d %d\n", pipe_fd[0], pipe_fd[1]);
    printf("pipe error file descriptors: %d %d\n", pipe_err_fd[0], pipe_err_fd[1]);

    read_input_from_file();

    reduced_input = minimize(original_input);

    printf(">>%s\n", reduced_input);
    printf("Bye!\n");
    return 0;
}
