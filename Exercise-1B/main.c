#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#define PI 3.141592654
#define PIPE_EVEN_WRITE 0
#define PIPE_EVEN_READ 1
#define PIPE_ODD_WRITE 2
#define PIPE_ODD_READ 3

int initialNumbersCapacity = 1;

static double complex convertInputToNumber(char *input);

static int readInput(double complex **numbers);

static void createChildProcesses(int pipes[4][2], int *evenProcess, int *oddProcess);

static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex *numbers);

static void waitForChildProcesses(const pid_t *evenProcess, const pid_t *oddProcess);

static void handleChildProcessResponse(int numbersAmount, FILE *fileEven, FILE *fileOdd);

static double roundToDecimals(double input);

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
    programmName = argv[0];
    // Parse command-line options if needed
    if (argc > 1 && strcmp(argv[1], "-p") == 0) {
        precision = 3;
    }
    double complex *numbers = (double complex *) calloc(initialNumbersCapacity, sizeof(double complex));
    if (numbers == NULL) {
        fprintf(stderr, "Something went wrong allocating memory\n");
        exit(EXIT_FAILURE);
    }

    int numbersAmount = readInput(&numbers);

    // Just print the number
    if (numbersAmount == 1) {
        double real = roundToDecimals(creal(numbers[0]));
        double imag = roundToDecimals(cimag(numbers[0]));
        printf("%.*f %.*f*i\n", precision, real, precision, imag);
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

    pid_t evenProcess;
    pid_t oddProcess;
    createChildProcesses(pipes, &evenProcess, &oddProcess);

    close(pipes[PIPE_EVEN_WRITE][0]);
    close(pipes[PIPE_EVEN_READ][1]);
    close(pipes[PIPE_ODD_WRITE][0]);
    close(pipes[PIPE_ODD_READ][1]);

    writeToChildProcesses(pipes, numbersAmount, numbers);
    free(numbers);

    close(pipes[PIPE_EVEN_WRITE][1]);
    close(pipes[PIPE_ODD_WRITE][1]);

    // wait for child processes
    waitForChildProcesses(&evenProcess, &oddProcess);

    FILE *evenFile = fdopen(pipes[PIPE_EVEN_READ][0], "r"); //opens pipe as file
    FILE *oddFile = fdopen(pipes[PIPE_ODD_READ][0], "r"); //opens pipe as file

    close(pipes[PIPE_EVEN_WRITE][0]);
    close(pipes[PIPE_ODD_WRITE][0]);

    handleChildProcessResponse(numbersAmount, evenFile, oddFile);

    fclose(evenFile);
    fclose(oddFile);
    exit(EXIT_SUCCESS);
}

/**
 * Performs butterfly calculation.
 * @param numbersAmount Amount of numbers in input
 * @param fileEven FILE-stream of even pipe
 * @param fileOdd FILE-stream of odd pipe
 * @return EXIT_FAILURE if child termination was not successful.
 */
static void handleChildProcessResponse(int numbersAmount, FILE *fileEven, FILE *fileOdd) {
    double complex *newNumbers = malloc(
            sizeof(double complex) * numbersAmount); //saves new calculated numbers to print later

    char *oddLine = NULL;
    char *evenLine = NULL;

    size_t oddLength = 0;
    size_t evenLength = 0;

    for (int k = 0; k < numbersAmount / 2; k++) {
        double complex t = (cos((((-2 * PI) / numbersAmount)) * k) + I * sin(((-2 * PI) / numbersAmount) * k));

        getline(&evenLine, &evenLength, fileEven);
        double complex even = convertInputToNumber(evenLine);

        getline(&oddLine, &oddLength, fileOdd);
        double complex odd = convertInputToNumber(oddLine);

        newNumbers[k] = even + t * odd;
        newNumbers[k + numbersAmount / 2] = even - t * odd;
    }
    free(oddLine);
    free(evenLine);

    for (int i = 0; i < numbersAmount; i++) {
        double real = creal(newNumbers[i]);
        if (fabs(real) < 0.000001) {
            real = 0.0;
        }

        double imag = cimag(newNumbers[i]);
        if (fabs(imag) < 0.000001) {
            imag = 0;
        }

        printf("%.*f %.*f*i\n", precision, real, precision, imag);
    }

    free(newNumbers);
}

/**
 * Waits for children to finish their process.
 * @param evenProcess ProcessID of evenProcess child
 * @param oddProcess ProcessID of oddProcess child
 * @return EXIT_FAILURE if child termination was not successful.
 */
static void waitForChildProcesses(const pid_t *evenProcess, const pid_t *oddProcess) {
    int status;
    pid_t terminatedChildPID = waitpid(*evenProcess, &status, 0);
    if (terminatedChildPID == -1) {
        fprintf(stderr, "child failed to terminate: evenProcess\n");
        exit(EXIT_FAILURE);
    }

    // Error in child
    if (WIFEXITED(status) != 1) {
        fprintf(stderr, "Even Child-Prozess %d wurde mit einem unbekannten Status beendet: %d\n", terminatedChildPID,
                status);
        exit(EXIT_FAILURE);
    }

    terminatedChildPID = waitpid(*oddProcess, &status, 0);
    if (terminatedChildPID == -1) {
        fprintf(stderr, "child failed to terminate: oddProcess\n");
        exit(EXIT_FAILURE);
    }

    // Error in child
    if (WIFEXITED(status) != 1) {
        fprintf(stderr, "Odd Child-Prozess %d wurde mit einem unbekannten Status beendet: %d\n", terminatedChildPID,
                status);
        exit(EXIT_FAILURE);
    }
}

/**
 * Writes input to child processes.
 * @brief Writes the input of the parent process to the stdin of the child process.
 * @param pipes Pipes of children
 * @param numbersAmount amount of numbers
 * @param numbers Pointer to numbers
 */
static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex *numbers) {
    char floatToString[100000];
    for (int i = 0; i < numbersAmount; i++) {
        double real = roundToDecimals(creal(numbers[i]));
        double imag = roundToDecimals(cimag(numbers[i]));

        snprintf(floatToString, sizeof(floatToString), "%.*f %.*f*i\n", precision, real, precision,
                 imag);
        write(pipes[i % 2 == 0 ? PIPE_EVEN_WRITE : PIPE_ODD_WRITE][1], floatToString, strlen(floatToString));
    }
}

/**
 * Creates child processes.
 * @brief Creates two child processes and redirected the stdin and stdout of the processes to the pipes..
 * @param pipes Pipes for redirecting child process inputs/outputs
 * @return EXIT_FAILURE if there is any problem with fork or dup2.
 */
static void createChildProcesses(int pipes[4][2], int *evenProcess, int *oddProcess) {
    *evenProcess = fork();
    if (*evenProcess == -1) {
        fprintf(stderr, "Fork for evenProcess failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*evenProcess == 0) {
        if(dup2(pipes[PIPE_EVEN_READ][1], STDOUT_FILENO) == -1 || dup2(pipes[PIPE_EVEN_WRITE][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "failure in creating even dup2");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < 4; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for evenProcess.\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    *oddProcess = fork();
    if (*oddProcess == -1) {
        fprintf(stderr, "Fork for oddProcess failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*oddProcess == 0) {
        if(dup2(pipes[PIPE_ODD_READ][1], STDOUT_FILENO) == -1 || dup2(pipes[PIPE_ODD_WRITE][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "failure in creating odd dup2");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < 4; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for oddProcess.\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
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

    int counter = 0;
    while ((getline(&line, &len, stdin)) != -1) {
        (*numbers)[counter] = convertInputToNumber(line);
        counter++;

        if (counter >= initialNumbersCapacity) {
            initialNumbersCapacity *= 2;
            *numbers = realloc(*numbers, initialNumbersCapacity * sizeof(double complex));

            if (*numbers == NULL) {
                fprintf(stderr, "Something went wrong allocating memory\n");
                free(*numbers);
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
    double real;
    double imag;

    // Verarbeiten Sie den Eingabestring
    char *endptrReal;

    // Suchen Sie nach dem Realteil
    real = strtod(input, &endptrReal);
    if (endptrReal == input) {
        fprintf(stderr, "Conversion of real number failed: Line entered: '%s'.\n", input);
        exit(EXIT_FAILURE);
    }

    real = roundToDecimals(real);

    // Überspringen Sie Leerzeichen und eventuelles Minuszeichen
    while (*endptrReal == ' ') {
        endptrReal++;
    }

    // there is no further string after empty spaces after number
    if (*endptrReal == '\0' || *endptrReal == '\n') {
        double complex z = real;
        return z;
    }

    // next non-empty char is not a number -->
    if(!(*endptrReal >= '0' && *endptrReal <= '9') && *endptrReal != '-') {
        fprintf(stderr, "Received false imaginary part.\n");
        exit(EXIT_FAILURE);
    }

    char *endptrImag;
    // check if there is another number - if not just return the real part of the number
    imag = strtod(endptrReal, &endptrImag);
    if (endptrReal == endptrImag || imag == 0) {
        double complex z = real;
        return z;
    }

    // check if there is a *i at the end
    if (*endptrImag != '*' || *(endptrImag + 1) != 'i') {
        fprintf(stderr, "Conversion of imaginary number failed - no trailing *i.\n");
        exit(EXIT_FAILURE);
    }

    endptrImag += 2;

    // there are further chars after *i
    if (*endptrImag != '\0' && *endptrImag != '\n') {
        fprintf(stderr, "Conversion of imaginary number failed - no newline after *i\n");
        exit(EXIT_FAILURE);
    }

    imag = roundToDecimals(imag);
    double complex z = real + imag * I;
    return z;
}

static double roundToDecimals(double input) {
    double decimals = pow(10, precision);
    return round(input * decimals) / decimals;
}

