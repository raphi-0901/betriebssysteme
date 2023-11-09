#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <errno.h>

#define PI 3.141592654
#define PIPE_EVEN_WRITE 0
#define PIPE_EVEN_READ 1
#define PIPE_ODD_WRITE 2
#define PIPE_ODD_READ 3

int initialNumbersCapacity = 1;

static double complex convertInputToNumber(char *input);

static int readInput(double complex **numbers);

static void createChildProcesses(int pipes[4][2], int *even, int *odd);

static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex *numbers);

static void waitForChildProcesses(pid_t *even, pid_t *odd);

static void handleChildProcessResponse(int numbersAmount, FILE *fileEven, FILE *fileOdd);

char *programmName;
int precision = 6;

/**
 * Program entry point.
 * @brief Reads input numbers and tries to convert it to complex numbers.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char **argv) {
    fprintf(stderr, "started process\n");
    programmName = argv[0];
    // Parse command-line options if needed
    if (argc > 1 && strcmp(argv[1], "-p") == 0) {
        precision = 3;
    }
    double complex *numbers = (double complex *) calloc(initialNumbersCapacity, sizeof(double complex));
    if (numbers == NULL) {
        printf("Something went wrong allocating memory\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "alive in line 55\n");
    int numbersAmount = readInput(&numbers);
    fprintf(stderr, "alive after line 55\n");
    fprintf(stderr, "%p\n", numbers);
    fprintf(stderr, "%d\n", numbersAmount);

    // Just print the number
    if (numbersAmount == 1) {
        printf("%.*f %.*f*i\n", precision, creal(numbers[0]), precision, cimag(numbers[0]));
        free(numbers);
        exit(EXIT_SUCCESS);
    }

    // Check for valid input
    if (numbersAmount <= 0 || numbersAmount % 2 != 0 || (numbersAmount & (numbersAmount - 1)) != 0) {
        fprintf(stderr, "Invalid amount of numbers in input:  %d.\n", numbersAmount);
        free(numbers);
        exit(EXIT_FAILURE);
    }

    // create pipes for children
    int pipes[4][2];
    for (int i = 0; i < 4; i++) {
        // error in pipe creation
        if (pipe(pipes[i]) == -1) {
            fprintf(stderr, "Pipe creation failure.\n");
            free(numbers);
            exit(EXIT_FAILURE);
        }
    }

    pid_t even;
    pid_t odd;
    writeToChildProcesses(pipes, numbersAmount, numbers);
    createChildProcesses(pipes, &even, &odd);
    free(numbers);

    // wait for child processes
    waitForChildProcesses(&even, &odd);

    FILE *fileE = fdopen(pipes[PIPE_EVEN_READ][0], "r"); //opens pipe as file
    FILE *fileO = fdopen(pipes[PIPE_ODD_READ][0], "r"); //opens pipe as file

    handleChildProcessResponse(numbersAmount, fileE, fileO);

    fclose(fileE);
    fclose(fileO);
}

/**
 * Performs butterfly calculation.
 * @param numbersAmount Amount of numbers in input
 * @param fileEven FILE-stream of even pipe
 * @param fileOdd FILE-stream of odd pipe
 * @return EXIT_FAILURE if child termination was not successful.
 */
static void handleChildProcessResponse(int numbersAmount, FILE *fileEven, FILE *fileOdd) {
    double complex *newNumbers = malloc(sizeof(double complex) * (numbersAmount / 2)); //saves new calculated numbers to print later

    char *oddLine = NULL;
    char *evenLine = NULL;

    size_t oddLength = 0;
    size_t evenLength = 0;

    for (int k = 0; k < numbersAmount / 2; k++) {
        float complex t = (cos((((-2 * PI) / numbersAmount)) * k) + I * sin(((-2 * PI) / numbersAmount) * k));

        getline(&evenLine, &evenLength, fileEven);
        double complex even = convertInputToNumber(evenLine);

        getline(&oddLine, &oddLength, fileOdd);
        double complex odd = convertInputToNumber(oddLine);

        newNumbers[k] = even + t * odd;
        newNumbers[k + numbersAmount / 2] = even - t * odd;
    }
    free(oddLine);
    free(evenLine);

    for (int i = 0; i < numbersAmount / 2; i++) {
        fprintf(stdout, "%.*f %.*f*i\n", precision, creal(newNumbers[0]), precision, cimag(newNumbers[0]));
    }

    free(newNumbers);
}

/**
 * Waits for children to finish their process.
 * @param even ProcessID of even child
 * @param odd ProcessID of odd child
 * @return EXIT_FAILURE if child termination was not successful.
 */
static void waitForChildProcesses(pid_t *even, pid_t *odd) {
    int status;
    pid_t terminatedChildPID = waitpid(*even, &status, 0);
    if (terminatedChildPID == -1) {
        perror("child failed to terminate: even\n");
        exit(EXIT_FAILURE);
    }
    // Error in child
    if (WIFEXITED(status) != 1) {
        printf("Child-Prozess %d wurde mit einem unbekannten Status beendet\n", terminatedChildPID);
    }

    terminatedChildPID = waitpid(*odd, &status, 0);
    if (terminatedChildPID == -1) {
        perror("child failed to terminate: odd\n");
        exit(EXIT_FAILURE);
    }

    // Error in child
    if (WIFEXITED(status) != 1) {
        printf("Child-Prozess %d wurde mit einem unbekannten Status beendet\n", terminatedChildPID);
    }
}

/**
 * Writes input to child processes.
 * @brief Writes the input of the parent process to the stdin of the child process.
 * @param pipes Pipes of children
 * @param numbersAmount amount of numbers
 * @param numbers Pointer to numbers
 */
static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex* numbers) {
    char floatToString[100000];
    printf("write to childs");
    for (int i = 0; i < numbersAmount; i++) {
        snprintf(floatToString, sizeof(floatToString), "%.*f %.*f*i\n", precision, creal(numbers[i]), precision, cimag(numbers[i]));
        write(pipes[i % 2 == 0 ? PIPE_EVEN_WRITE : PIPE_ODD_WRITE][1], floatToString, strlen(floatToString));
    }
}

/**
 * Creates child processes.
 * @brief Creates two child processes and redirected the stdin and stdout of the processes to the pipes..
 * @param pipes Pipes for redirecting child process inputs/outputs
 * @return EXIT_FAILURE if there is any problem with fork or dup2.
 */
static void createChildProcesses(int pipes[4][2], int *even, int *odd) {
    *even = fork();
    if (*even == -1) {
        fprintf(stderr, "Fork for even failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*even == 0) {
        //printf("duplicate even pipes\n");

        // close unused pipes
        close(pipes[PIPE_ODD_READ][0]);
        close(pipes[PIPE_ODD_WRITE][0]);
        close(pipes[PIPE_ODD_READ][1]);
        close(pipes[PIPE_ODD_WRITE][1]);

        close(pipes[PIPE_EVEN_READ][1]);
        close(pipes[PIPE_EVEN_WRITE][0]);

        if(dup2(pipes[PIPE_EVEN_WRITE][1], STDOUT_FILENO) == -1 || dup2(pipes[PIPE_EVEN_READ][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "Error in dup2 for even.\n");
            exit(EXIT_FAILURE);
        }
        close(pipes[PIPE_EVEN_READ][0]);
        close(pipes[PIPE_EVEN_WRITE][1]);

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for even.\n");
            exit(EXIT_FAILURE);
        }
        return;
    }

    *odd = fork();
    if (*odd == -1) {
        fprintf(stderr, "Fork for odd failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*odd == 0) {
        //printf("duplicate odd pipes\n");

        // close unused pipes
        close(pipes[PIPE_EVEN_READ][0]);
        close(pipes[PIPE_EVEN_WRITE][0]);
        close(pipes[PIPE_EVEN_READ][1]);
        close(pipes[PIPE_EVEN_WRITE][1]);

        close(pipes[PIPE_ODD_READ][1]);
        close(pipes[PIPE_ODD_WRITE][0]);

        if(dup2(pipes[PIPE_ODD_WRITE][1], STDOUT_FILENO) == -1 || dup2(pipes[PIPE_ODD_READ][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "Error in dup2 for odd.\n");
            exit(EXIT_FAILURE);
        }
        close(pipes[PIPE_ODD_READ][0]);
        close(pipes[PIPE_ODD_WRITE][1]);

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for odd.\n");
            exit(EXIT_FAILURE);
        }
        return;
    }

    // close pipes
    for(int i = 0; i < 4; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

/**
 * Reads input.
 * @brief Reads the input from stdin and tries to convert it to a complex double. If this successfully converted, it gets added to the number array.
 * @details Exits the program if the input does not match a real or imaginary number.
 * @param numbers Pointer to numbers array
 * @return amount of numbers saved in numbers. If there is a problem in converting the input, EXIT_FAILURE gets returned.
 */
static int readInput(double complex *numbers[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    int counter = 0;
    while ((nread = getline(&line, &len, stdin)) != -1) {
        (*numbers)[counter] = convertInputToNumber(line);
        counter++;

        if (counter > 3) {
            break;
        }

        if (counter >= initialNumbersCapacity) {
            initialNumbersCapacity *= 2;
            *numbers = realloc(*numbers, initialNumbersCapacity * sizeof(double complex));

            if (*numbers == NULL) {
                fprintf(stderr, "Something went wrong allocating memory\n");
                free(numbers);
                free(line);
                exit(EXIT_FAILURE);
            }
        }
    }
    free(line);
    return counter;
}

/**
 * @brief Converts an input line to a double complex number.
 * @details does not allow letters in between, if there is a second number, it has to end with *i\n.
 * Following inputs are getting converted (comma-separated means \n): 12; -0.43 4*i;
 * This ones are not getting matched: 12s; -0.43 4; 6*i; 3 6*I; -4 -4.234*isdf;
 * @param input Input to convert
 * @return converted complex number. If input does not allow to convert, EXIT_FAILURE gets returned
 */
static double complex convertInputToNumber(char *input) {
    // Variablen für die Real- und Imaginärteile
    double real = 0.0;
    double imag = 0.0;

    // Verarbeiten Sie den Eingabestring
    char *endptrReal;

    // Suchen Sie nach dem Realteil
    real = strtod(input, &endptrReal);

    if (endptrReal == input || real == 0.0) {
        printf("Conversion of real number failed.\n");
        exit(EXIT_FAILURE);
    }

    // Überspringen Sie Leerzeichen und eventuelles Minuszeichen
    while (*endptrReal == ' ') {
        endptrReal++;
    }

    // there is no further string after empty spaces after number
    if (*endptrReal == '\0' || *endptrReal == '\n') {
        double complex z = real + 0 * I;
        return z;
    }

    char *endptrImag;
    // check if there is another number
    imag = strtod(endptrReal, &endptrImag);
    if (endptrReal == endptrImag || imag == 0.0) {
        printf("Conversion of imaginary number failed.\n");
        exit(EXIT_FAILURE);
    }

    // check if there is a *i at the end
    if (*endptrImag != '*' || *(endptrImag + 1) != 'i') {
        printf("Conversion of imaginary number failed - no trailing *i.\n");
        exit(EXIT_FAILURE);
    }

    endptrImag += 2;

    // there are further chars after *i
    if (*endptrImag != '\0' && *endptrImag != '\n') {
        printf("Conversion of imaginary number failed - no newline after *i\n");
        exit(EXIT_FAILURE);
    }

    double complex z = real + imag * I;
    return z;
}