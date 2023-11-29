#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int opt;
    char *inputFile = NULL;
    while ((opt = getopt(argc, argv, "i:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            inputFile = optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind + 2 != argc)
    {
        fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    char *cmd1 = argv[optind++];
    char *cmd2 = argv[optind++];
    FILE *fInputFile;
    if (inputFile != NULL)
    {
        fInputFile = fopen(inputFile, "r");

        if (fInputFile == NULL)
        {
            return 1;
        }
        fclose(fInputFile);
    }

    /* Other code omitted */

    int pipes[2][2];

    for (size_t i = 0; i < 2; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }

    int pid1 = fork();
    if (pid1 == 0)
    {
        close(pipes[0][0]);
        if (dup2(pipes[0][1], STDOUT_FILENO) == -1)
        {
            exit(EXIT_FAILURE);
        }
        close(pipes[0][1]);
        // dup2
        execlp(cmd1, cmd1, NULL);
    }
    close(pipes[0][1]);

    int pid2 = fork();
    if (pid2 == 0)
    {
        close(pipes[1][0]);
        if (dup2(pipes[1][1], STDOUT_FILENO) == -1)
        {
            exit(EXIT_FAILURE);
        }
        close(pipes[1][1]);
        // dup2
        execlp(cmd2, cmd2, NULL);
    }
    close(pipes[1][1]);

    int status;
    int w;
    do
    {
        w = waitpid(pid1, &status, 0);
        if (w == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status))
        {
            printf("exited, status=%d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("killed by signal %d\n", WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            printf("stopped by signal %d\n", WSTOPSIG(status));
        }
        else if (WIFCONTINUED(status))
        {
            printf("continued\n");
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    int status2;
    do
    {
        w = waitpid(pid2, &status2, 0);
        if (w == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status2))
        {
            printf("exited, status2=%d\n", WEXITSTATUS(status2));
        }
        else if (WIFSIGNALED(status2))
        {
            printf("killed by signal %d\n", WTERMSIG(status2));
        }
        else if (WIFSTOPPED(status2))
        {
            printf("stopped by signal %d\n", WSTOPSIG(status2));
        }
        else if (WIFCONTINUED(status2))
        {
            printf("continued\n");
        }
    } while (!WIFEXITED(status2) && !WIFSIGNALED(status2));

    FILE *output1 = fdopen(pipes[0][0], "r");
    FILE *output2 = fdopen(pipes[1][0], "r");
    // close(pipes[0][0]);
    int nread;
    char *line = NULL;
    size_t len = 0;
    printf("before reading\n");

    while ((nread = getline(&line, &len, output1)) != -1)
    {
        fwrite(line, nread, 1, stdout);
    }

    printf("before reading\n");

    while ((nread = getline(&line, &len, output2)) != -1)
    {
        fwrite(line, nread, 1, stdout);
    }

    free(line);
    fclose(output1);
    fclose(output2);

    exit(EXIT_SUCCESS);
}