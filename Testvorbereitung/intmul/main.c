#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *programName = "empty";

int checkForHexadecimal(char *input);
void createChildProcesses(int pipes[8][2], int childProcessIds[4]);
static void writeToChildProcesses(int pipes[8][2], int inputs[2]);
static void splitInputIntoParts(int input[2], int parts[2][2]);

void usage(void)
{
    fprintf(stderr, "Usage: %s\n", programName);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    programName = argv[0];
    size_t len = 0;
    if (argc != 1)
    {
        usage();
    }

    int inputs[2];
    for (size_t i = 0; i < 2; i++)
    {
        char *input = NULL;
        size_t read = 0;
        if ((read = getline(&input, &len, stdin)) == -1)
        {
            fprintf(stderr, "no two numbers set\n");
            usage();
        }

        if (input[read - 1] == '\n')
        {
            input[read - 1] = '\0';
        }

        if (checkForHexadecimal(input) == -1)
        {
            fprintf(stderr, "no hexadecimal numbers set\n");
            usage();
        }

        inputs[i] = strtol(input, NULL, 16);
    }

    // if only one digit per input, simply multiply them and print them
    if (inputs[0] < 16 && inputs[1] < 16)
    {
        fprintf(stdout, "%X\n", inputs[0] * inputs[1]);
        exit(EXIT_SUCCESS);
    }

    int parts[2][2];
    // split in parts
    splitInputIntoParts(inputs, parts);

    // create pipes for children
    int pipes[8][2];
    for (int i = 0; i < 8; i++)
    {
        // error in pipe creation
        if (pipe(pipes[i]) == -1)
        {
            fprintf(stderr, "Pipe creation failure.\n");
            exit(EXIT_FAILURE);
        }
    }

    int childProcessIds[4];
    createChildProcesses(pipes, childProcessIds);

    // write to childs
    writeToChildProcesses(pipes, inputs);

    FILE *files[4];
    for (size_t i = 0; i < 4; i++)
    {
        files[i] = fdopen(*pipes[i], "r");
    }

    for (size_t i = 0; i < 2; i++)
    {
    }

    exit(EXIT_SUCCESS);
    return 0;
}

void createChildProcesses(int pipes[8][2], int childProcessIds[4])
{
    for (size_t i = 0; i < 4; i++)
    {
        childProcessIds[i] = fork();
        if (childProcessIds[i] == -1)
        {
            fprintf(stderr, "Fork for evenProcess failed.\n");
            exit(EXIT_FAILURE);
        }

        if (childProcessIds[i] == 0)
        {
            if (dup2(pipes[2 * i][1], STDOUT_FILENO) == -1 ||
                dup2(pipes[(2 * i) + 1][0], STDIN_FILENO) == -1)
            {
                fprintf(stderr, "failure in creating even dup2");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < 8; i++)
            {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            if (execlp(programName, programName, NULL) == -1)
            {
                fprintf(stderr, "Error in execlp for evenProcess.\n");
                exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
        }
    }
}

static void writeToChildProcesses(int pipes[8][2], int parts[2][2])
{
    for (int i = 0; i < 4; i++)
    {
        // fill in
        write(pipes[(2 * i) + 1][1], parts[0][0], sizeof(int));
    }
}

static void splitInputIntoParts(int input[2], int parts[2][2])
{
    for (size_t i = 0; i < 2; i++)
    {
        char *hexString = malloc(sizeof(int));
        sprintf(hexString, "%d", input[i]);

        int inputLength = strlen(hexString);

        if (inputLength % 2 != 0)
        {
            for (int i = inputLength; i >= 0; i--)
            {
                hexString[i + 1] = hexString[i];
            }
            hexString[0] = '0';
        }

        char left[inputLength / 2];
        char right[inputLength / 2];

        for (int i = 0; i < inputLength / 2; i++)
        {
            left[i] = hexString[i];
            right[(inputLength / 2) + i] = hexString[i];
        }

        parts[i][0] = strtol(left, NULL, 16);
        parts[i][0] = strtol(right, NULL, 16);
    }
}

int checkForHexadecimal(char *input)
{
    for (int i = 0; i < strlen(input); i++)
    {
        char c = input[i];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
        {
            continue;
        }

        return -1;
    }
    return 0;
}