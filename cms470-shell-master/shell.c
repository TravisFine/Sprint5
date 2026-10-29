/*
 * CMS 470 - Sprint 5 shell
 * References used for the concepts and library functions:
 * https://brennan.io/2015/01/16/write-a-shell-in-c/
 * https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf
 * https://man7.org/linux/man-pages/man3/getline.3.html
 * https://man7.org/linux/man-pages/man3/strtok.3.html
 * https://man7.org/linux/man-pages/man3/getcwd.3.html
 * https://man7.org/linux/man-pages/man2/chdir.2.html
 * https://man7.org/linux/man-pages/man2/open.2.html
 * https://man7.org/linux/man-pages/man2/dup.2.html
 * Also see the two example programs provided with the assignment.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* Replace standard output with the requested file. */
int redirect_output(char *filename) {
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        perror(filename);
        return -1;
    }
    if (dup2(file, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(file);
        return -1;
    }
    if (file != STDOUT_FILENO) {
        close(file);
    }
    return 0;
}

int main(void) {
    char *line = NULL;
    size_t capacity = 0;
    int running = 1;
    int status = 0;

    while (running) {
        printf("My Shell > ");
        fflush(stdout);
        if (getline(&line, &capacity, stdin) == -1) {
            if (!feof(stdin)) {
                perror("getline");
                status = 1;
            }
            break;
        }

        /* Space out > so both "ls > file" and "ls>file" work.
         * Quoted strings and escape sequences are outside this assignment.
         */
        size_t length = strlen(line);
        char *input = malloc(length * 3 + 1);
        char **args = malloc((length + 1) * sizeof(char *));
        if (input == NULL || args == NULL) {
            fprintf(stderr, "Not enough memory for this command.\n");
            free(input);
            free(args);
            status = 1;
            break;
        }
        size_t j = 0;
        for (size_t i = 0; i < length; i++) {
            if (line[i] == '>') {
                input[j++] = ' ';
                input[j++] = '>';
                input[j++] = ' ';
            } else {
                input[j++] = line[i];
            }
        }
        input[j] = '\0';

        int count = 0;
        int invalid = 0;
        char *output = NULL;
        char *token = strtok(input, " \t\r\n");
        while (token != NULL) {
            if (strcmp(token, ">") == 0) {
                if (output != NULL) {
                    invalid = 1;
                    break;
                }
                output = strtok(NULL, " \t\r\n");
                if (output == NULL || strcmp(output, ">") == 0) {
                    invalid = 1;
                    break;
                }
            } else {
                args[count++] = token;
            }
            token = strtok(NULL, " \t\r\n");
        }
        args[count] = NULL;

        if (invalid || (count == 0 && output != NULL)) {
            fprintf(stderr, "Use: command [arguments] > filename\n");
        } else if (count > 0) {
            int builtin = strcmp(args[0], "exit") == 0 ||
                          strcmp(args[0], "pwd") == 0 ||
                          strcmp(args[0], "cd") == 0;

            if (builtin) {
                /* Built-ins run here so cd changes the shell's directory.
                 * Save stdout so a redirected pwd does not hide later prompts.
                 */
                int saved = -1;
                int ready = 1;
                if (output != NULL) {
                    fflush(stdout);
                    saved = dup(STDOUT_FILENO);
                    if (saved == -1) {
                        perror("dup");
                        ready = 0;
                    } else if (redirect_output(output) == -1) {
                        ready = 0;
                    }
                }

                if (ready) {
                    if (strcmp(args[0], "exit") == 0) {
                        running = 0;
                    } else if (strcmp(args[0], "pwd") == 0) {
                        char *directory = getcwd(NULL, 0);
                        if (directory == NULL) {
                            perror("pwd");
                        } else {
                            printf("%s\n", directory);
                            free(directory);
                        }
                    } else if (count != 2) {
                        fprintf(stderr, "Use: cd directory\n");
                    } else if (chdir(args[1]) == -1) {
                        perror("cd");
                    }
                }

                if (saved != -1) {
                    fflush(stdout);
                    if (dup2(saved, STDOUT_FILENO) == -1) {
                        perror("restore stdout");
                        running = 0;
                        status = 1;
                    }
                    close(saved);
                }
            } else {
                pid_t child = fork();
                if (child == -1) {
                    perror("fork");
                } else if (child == 0) {
                    if (output != NULL && redirect_output(output) == -1) {
                        free(args);
                        free(input);
                        free(line);
                        return 1;
                    }
                    execvp(args[0], args);
                    /* A successful execvp never returns. */
                    perror(args[0]);
                    free(args);
                    free(input);
                    free(line);
                    return 1;
                } else {
                    /* Retry if a signal interrupts the wait. */
                    while (waitpid(child, NULL, 0) == -1) {
                        if (errno != EINTR) {
                            perror("waitpid");
                            break;
                        }
                    }
                }
            }
        }
        free(args);
        free(input);
    }

    free(line);
    return status;
}
