#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>


char *reduce(char *t);

#define BUFFER_SIZE 1024

int pipe_fd[2];   // pipe file descriptors
pid_t child_pid; // child process id
char buffer[BUFFER_SIZE];
char *input_path = NULL;
char *output_file_path = NULL;
char *error_string = NULL;

char **program_args;

char *input;
char *reduced_input;

int output_fd;
int does_contain_error_msg()
{
    return strstr(buffer, error_string) != NULL;
}
int execute_target_program(char *tm)
{
    int flag;
    FILE* input_fp = fopen(input_path, "rb");
    int input_fd = fileno(input_fp);
    if(dup2(input_fd, 0) == -1){
        fprintf(stderr, "Error dup2: %s\n", strerror(errno));
    }
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

int child_process(char *test)
{
    // redirect stdout to pipe
    // 0 - stdin, the standard input stream.
    // 1 - stdout, the standard output stream.
    // 2 - stderr, the standard error stream.
    dup2(pipe_fd[1], 2);
    // execute program with arguments
    int flag = execute_target_program(test);
    if (flag == -1)
    {
        fprintf(stderr, "Error executing program: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
   
}
char * parent_process1(char *head, char *tail, char *test)
{
    // close write end of pipe
    close(pipe_fd[1]);

    // read from pipe until error string is found or end of file
    int bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE-1)) > 0)
    {
        buffer[bytes_read+1] = 0x0;
        if (does_contain_error_msg()) // check if error string is found
        {
            kill(child_pid, SIGINT);
            char *reduced = reduce(test);
            free(test);
            free(head);
            free(tail);
            return reduced;
        }
    }
    // error string not found
    fprintf(stderr, "Error string not found in output\n");

    // exit parent process
    exit(EXIT_FAILURE);
}
char * parent_process2(char *mid)
{
    // close write end of pipe
    close(pipe_fd[1]);

    // read from pipe until error string is found or end of file
    int bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE)) > 0)
    {
        if (does_contain_error_msg()) // check if error string is found
        {
            kill(child_pid, SIGINT);
            char *reduced = reduce(mid);
            free(mid);
            return reduced;
        }
    }
    // error string not found
    fprintf(stderr, "Error string not found in output\n");

    // exit parent process
    exit(EXIT_FAILURE);
}
int write_reduced_input_to_file(int reduced_input_size)
{
    // write reduced input to file
    FILE *output_file = fopen(output_file_path, "w");
    if (output_file == NULL)
    {
        fprintf(stderr, "Error opening output file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fwrite(reduced_input, 1, reduced_input_size, output_file);
    fclose(output_file);

    // print size of reduced input
    printf("Reduced input size: %d\n", reduced_input_size);

    // exit parent process
    exit(EXIT_SUCCESS);
}
int read_input_from_file(){
    FILE *fp;
    char *buf;
    long file_size;

    // Open the file in binary mode
    fp = fopen(input_path, "rb");

    if (fp == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    // Determine the size of the file
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for the buffer
    buf = (char*)malloc(file_size + 1);
    if (buf == NULL) {
        printf("Error allocating memory.\n");
        exit(1);
    }

    // Read the contents of the file into the buffer
    fread(buf, file_size, 1, fp);
    buf[file_size] = '\0'; // Add null terminator to end of buffer

    // Close the file
    fclose(fp);

    // save the contents of the buffer
    input = (char*)malloc(file_size + 1);
    strcpy(input, buf);

    // Free the memory allocated for the buffer
    free(buf);
    return 0;
}

// 𝑝, a target programt that crashes p
// 𝑐, a predicate over output to check if the program crashed
char *reduce(char *t)
{
    char *tm = t;
    int s = strlen(tm) - 1;
    while (s > 0)
    {
        for (int i = 0; i < strlen(tm) - s; i++)
        {
            char *head = strndup(tm, i); // The strndup(s, n) function copies at most n bytes. If s is longer than n, only n bytes are copied, and a terminating null byte('\0') is added.
            char *tail = strndup(&tm[i + s], strlen(tm) - i - s);
            char *test = malloc(strlen(head) + strlen(tail) + 1);
            strcpy(test, head);
            strcat(test, tail);

            child_pid = fork();

            if (child_pid == 0)
            { // child process
                child_process(test);
            }
            else
            { // parent process
                parent_process1(head, tail, test);
            }

            // free(test);
            // free(head);
            // free(tail);
        }

        for (int i = 0; i < strlen(tm) - s; i++)
        {
            char *mid = strndup(&tm[i], s);

            child_pid = fork();

            if (child_pid == 0)
            { // child process
                child_process(mid);
            }
            else
            { // parent process
                parent_process2(mid);
            }

            // free(mid);
        }

        s--;
    }
    return tm;
}

char *minimize(char *t)
{
    return reduce(t);
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
    close(output_fd);
    exit(0);
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
            input_path = optarg;
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
    if (input_path == NULL || error_string == NULL || output_file_path == NULL)
    {
        fprintf(stderr, "Usage: %s -i input_path -m error_string -o output_path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    program_args = &argv[optind]; // assume all the option arguments are handled.

    printf("\n--------------------------------\n");
    printf("input_path: %s\n", input_path);
    printf("error_string: %s\n", error_string);
    printf("output_path: %s\n", output_file_path);
    printf("target program path: %s\n", program_args[0]);
    printf("--------------------------------\n");
    // create pipe
    if (pipe(pipe_fd) == -1)
    {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("pipe file descriptors: %d %d\n", pipe_fd[0], pipe_fd[1]);

    read_input_from_file();
    reduced_input = minimize(input);

    printf(">>%s\n", reduced_input);
    printf("Bye!\n");
    return 0;
}
