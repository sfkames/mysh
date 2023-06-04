#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define input_delim " \t\r\n\v\f\174\074\076\052"

void execute_shell(char* input);
char **parse_input(char* input);
int cd_shell(char** parsed);
char **parse_pipe(char *input);
void execpipe(char **parsed_pipe);

int main(int argc, char *argv[])
{
    char *s = NULL;
    size_t n = 0;
    FILE* fp = NULL;

    // Batch mode
    if (argc == 2) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            perror("Error opening file\n");
            exit(EXIT_FAILURE);
        }
        while (1) {
            if (fp != NULL) {
                ssize_t line_size;
                line_size = getline(&s, &n, fp);

                while(line_size >= 0) {
                    execute_shell(s);
                    line_size = getline(&s, &n, fp);
                }
            }
            fclose(fp);
            return EXIT_SUCCESS;
        }
    } else {

        printf("Welcome to my shell!\n");

        while(1) {
            printf("mysh> ");

            getline(&s, &n, stdin);

            execute_shell(s);

            if(feof(stdin)) {
                break;
            }
        }
        return EXIT_SUCCESS;
    }
}

void execute_shell(char* input)
{
    char *command = (char*)malloc(1024);
    char **parsed;
    char **parsed1;
    pid_t pid;
    int status;

    if(!strcmp(input, "\n")) {
        return;
    }

    parsed1 = parse_pipe(input);

    if (parsed1[1] != NULL) {
        execpipe(parsed1);
    }
    free(parsed1);

    parsed = parse_input(input);

    command = strcpy(command, parsed[0]);

    if (!strcmp(command, "cd")) {
        cd_shell(parsed);
    } else if (!strcmp(command, "pwd")) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("%s\n", cwd);
    } else if (!strcmp(command, "exit")) {
        printf("exiting\n");
        exit(0);
    } else {
        // Child process
        if((pid = fork()) == 0) {
            // Execute user command
            execvp(parsed[0], parsed);
            // If command not found, print error statement
            printf("%s: command not found\n", parsed[0]);
            printf("!");
            free(command);
            free(parsed);
            return;
        } else if (pid < 0) {
            perror("fork() error");
        } else {
            // Parent process
            do {
                if (waitpid(pid, &status, WUNTRACED) == -1) {
                    perror("waitpid() error");
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
    free(command);
    free(parsed);
    return;
}

char **parse_input(char* input)
{
    int index = 0;
    char *rest = NULL;
    char **tokens = malloc(sizeof(char*) * 1024);
    char *token;

    token = strtok_r(input, input_delim, &rest);

    while (token != NULL) {
        tokens[index] = token;
        index++;

        token = strtok_r(NULL, input_delim, &rest);
    }
    tokens[index] = NULL;

    return tokens;
}

int cd_shell(char** parsed)
{
    if (parsed[1] == NULL) {
        chdir("HOME");
        setenv("PWD", "HOME", 1);
    } else {
        if (chdir(parsed[1]) != 0) {
            perror(parsed[1]);
            printf("!");
        } else {
            setenv("PWD", parsed[1], 1);
        }
    }
    return 1;
}

char **parse_pipe(char *input)
{
    char **tokens = malloc(sizeof(char*) * 1024);
    char *token;
    char *temp = strdup(input);

    token = strsep(&temp, "|");
    tokens[0] = token;
    tokens[1] = temp;

    return tokens;    
}

void execpipe(char **parsed_pipe)
{
    int fds[2];
    pid_t pid1 = fork();

    if (pipe(fds) < 0) {
        perror("pipe error");
        exit(1);
    }
    if (pid1 < 0) {
        perror("fork error");
        exit(1);
    } else if (pid1 == 0) {      // Child
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        
        if (execvp(parsed_pipe[0], parsed_pipe) < 0) {
            perror("command 1 error");
            exit(1);
        }
    } else {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);

            if (execvp(parsed_pipe[1], parsed_pipe) < 0) {
                perror("command 2 error");
                exit(1);
            }
        } else {
            close(fds[0]);
            close(fds[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
}