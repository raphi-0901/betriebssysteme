#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

char *programName;

static void createChildProcesses();

typedef struct Point
{
    double x;
    double y;
} point_t;

void error(char *error)
{
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

point_t parsePoint(char *line);

static void createChildProcesses(int pipes[4][2], int childProcessIds[2])
{
    for (int i = 0; i < 2; i++)
    {
        childProcessIds[i] = fork();

        if (childProcessIds[i] == -1)
        {
            error("childCreationFailed");
        }

        if (childProcessIds[i] == 0)
        {
            if (dup2(pipes[2 * i][0], STDIN_FILENO) == -1 || dup2(pipes[(2 * i) + 1][1], STDOUT_FILENO) == -1)
            {
                error("dup2 failed");
            }

            for (int j = 0; j < 4; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (execlp(programName, programName, NULL) == -1)
            {
                error("error in child");
            }
        }

        // close unused pipes
        close(pipes[(2 * i)][0]);
        close(pipes[(2 * i) + 1][1]);
    }
}

static void handleChildProcessResponse()
{
}

static void waitForChildren(int processIds[2])
{
    for (size_t i = 0; i < 2; i++)
    {
        int status;
        int terminatedChildPID = waitpid(processIds[i], &status, 0);
        if (terminatedChildPID == -1)
        {
            error("child failed to terminate: evenProcess");
        }
        waitpid(processIds[i], &status, 0);

        if (WIFEXITED(status) != 1)
        {
            error("problem in child");
        }
    }
}

static void writeToChildProcesses(int pipes[4][2], point_t points[], int counter)
{
    // calculate mean and split array in two parts
    double meanX = 0;

    for (int i = 0; i < counter; i++)
    {
        meanX += points[i].x;
    }

    meanX /= counter;

    point_t *lower = malloc(sizeof(point_t) * counter);
    point_t *upper = malloc(sizeof(point_t) * counter);

    int lowerCounter = 0;
    int upperCounter = 0;
    for (int i = 0; i < counter; i++)
    {
        if (points[i].x <= meanX)
        {
            lower[lowerCounter++] = points[i];
        }
        else
        {
            upper[upperCounter++] = points[i];
        }
    }

    for (int i = 0; i < 2; i++)
    {
        char output[1024];
        if (i % 2 == 0)
        {
            for (int j = 0; j < lowerCounter; j++)
            {
                sprintf(output, "%f %f\n", lower[j].x, lower[j].y);
                // fprintf(stderr, "%s", output);
                write(pipes[2 * i][1], output, strlen(output));
            }
        }
        else
        {
            for (int j = 0; j < upperCounter; j++)
            {
                sprintf(output, "%f %f\n", upper[j].x, upper[j].y);
                // fprintf(stderr, "%s", output);
                write(pipes[2 * i][1], output, strlen(output));
            }
        }
    }
}

int main(int argc, char **argv)
{
    programName = argv[0];
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int initialCapacity = 4;
    point_t *points = malloc(sizeof(point_t) * initialCapacity);
    if (points == NULL)
    {
        error("Failed allocating mememory");
    }

    int counter = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    fprintf(stderr, "wait for input\n");
    while ((nread = getline(&line, &len, stdin)) != -1)
    {
        // increase space for array
        if (counter >= initialCapacity)
        {
            initialCapacity *= 2;
            points = realloc(points, initialCapacity);
        }
        points[counter] = parsePoint(line);
        counter++;
    }
    free(line);

    // base case
    if (counter == 1)
    {
        exit(EXIT_SUCCESS);
    }

    // write to stdout
    if (counter == 2)
    {
        for (int i = 0; i < 2; i++)
        {
            fprintf(stdout, "%f %f\n", points[i].x, points[i].y);
        }
        exit(EXIT_SUCCESS);
    }

    int pipes[4][2];
    for (int i = 0; i < 4; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            error("Pipe creation failed");
        }
    }

    int childProcessIds[2];
    createChildProcesses(pipes, childProcessIds);
    writeToChildProcesses(pipes, points, counter);
    waitForChildren(childProcessIds);

    FILE *files[2];
    for (int i = 0; i < 2; i++)
    {
        files[i] = fdopen(pipes[2 * i][0], "r");
        close(pipes[2 * i][0]);
    }

    handleChildProcessResponse();

    return 0;
}

point_t parsePoint(char *line)
{
    point_t point;
    char *endptr;
    double value = 0;
    errno = 0; /* To distinguish success/failure after call */
    value = strtod(line, &endptr);
    /* Check for various possible errors. */
    if (errno != 0)
    {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (endptr == line)
    {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }

    while (*endptr == ' ')
    {
        endptr++;
    }

    errno = 0; /* To distinguish success/failure after call */
    point.x = value;
    char *endptr2;
    value = strtod(endptr, &endptr2);
    /* Check for various possible errors. */
    if (errno != 0)
    {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (endptr == endptr2)
    {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    point.y = value;
    return point;
}