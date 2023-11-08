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

int initialNumbersCapacity = 1;

static double complex convertInputToNumber(char *input);

static void fft(float complex A[], int n, int precision, int pipe_even[2], int pipe_odd[2]);

static int readInput(double complex **numbers);

static void createChildProcesses(int pipes[4][2], int *even, int *odd);

static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex *numbers);

static void waitForChildProcesses(pid_t *even, pid_t *odd);

static void handleChildProcessResponse(int numbersAmount, FILE *fileE, FILE *fileO);

char *programmName;
int precision = 6;

int main(int argc, char **argv) {
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

    int numbersCount = readInput(&numbers);

    // Just print the number
    if (numbersCount == 1) {
        printf("%.*f %.*f*i\n", precision, creal(numbers[0]), precision, cimag(numbers[0]));
        free(numbers);
        exit(EXIT_SUCCESS);
    }

    // Check for valid input
    if (numbersCount <= 0 || numbersCount % 2 != 0 || (numbersCount & (numbersCount - 1)) != 0) {
        fprintf(stderr, "Invalid amount of numbers in input:  %d.\n", numbersCount);
        free(numbers);
        exit(EXIT_FAILURE);
    }

    // create pipes for children
    int pipes[4][2];
    // pipe[0] -> even read
    // pipe[1] -> even write
    // pipe[2] -> odd read
    // pipe[3] -> odd write
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
    createChildProcesses(pipes, &even, &odd);
    writeToChildProcesses(pipes, numbersCount, numbers);
    free(numbers);

    // wait for child processes
    waitForChildProcesses(&even, &odd);

    FILE *fileE = fdopen(pipes[0][0], "r"); //opens pipe as file
    FILE *fileO = fdopen(pipes[1][0], "r"); //opens pipe as file

    handleChildProcessResponse(numbersCount, fileE, fileO);

    fclose(fileE);
    fclose(fileO);
}

static void handleChildProcessResponse(int numbersAmount, FILE *fileE, FILE *fileO) {
    double complex *newNumbers = malloc(sizeof(double complex) * (numbersAmount / 2)); //saves new calculated numbers to print later

    char *oddLine = NULL;
    char *evenLine = NULL;

    size_t oddLength = 0;
    size_t evenLength = 0;

    for (int k = 0; k < numbersAmount / 2; k++) {
        float complex t = (cos((((-2 * PI) / numbersAmount)) * k) + I * sin(((-2 * PI) / numbersAmount) * k));

        getline(&evenLine, &evenLength, fileE);
        double complex even = convertInputToNumber(evenLine);

        getline(&oddLine, &oddLength, fileO);
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

static void waitForChildProcesses(pid_t *even, pid_t *odd) {
    int status;
    waitpid(*even, &status, 0);
    pid_t terminatedChildPID = waitpid(*even, &status, 0);
    if (terminatedChildPID == -1) {
        perror("waitpid even\n");
        exit(EXIT_FAILURE);
    }
    // Error in child
    if (WIFEXITED(status) != 1) {
        printf("Child-Prozess %d wurde mit einem unbekannten Status beendet\n", terminatedChildPID);
    }

    waitpid(*odd, &status, 0);
    terminatedChildPID = waitpid(*odd, &status, 0);
    if (terminatedChildPID == -1) {
        perror("waitpid odd\n");
        exit(EXIT_FAILURE);
    }

    // Error in child
    if (WIFEXITED(status) != 1) {
        printf("Child-Prozess %d wurde mit einem unbekannten Status beendet\n", terminatedChildPID);
    }
}

static void writeToChildProcesses(int pipes[4][2], int numbersAmount, double complex* numbers) {
    char floatToString[100000];
    printf("write to childs");
    for (int i = 0; i < numbersAmount; i++) {
        snprintf(floatToString, sizeof(floatToString), "%.*f %.*f*i\n", precision, creal(numbers[i]), precision, cimag(numbers[i]));
        write(pipes[i % 2 == 0 ? 1 : 3][1], floatToString, strlen(floatToString));
    }
}

static void createChildProcesses(int pipes[4][2], int *even, int *odd) {
    *even = fork();
    if (*even == -1) {
        fprintf(stderr, "Fork for even failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*even == 0) {
        printf("duplicate even pipes\n");

        // need write pipe on one hand
        if(dup2(pipes[0][1], STDOUT_FILENO) == -1 || dup2(pipes[1][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "Error in dup2 for even.\n");
            exit(EXIT_FAILURE);
        }

        // dup2 pipes
//        if (dup2(pipes[0][0], STDIN_FILENO) == -1 || dup2(pipes[0][1], STDOUT_FILENO) == -1) {
//            fprintf(stderr, "Error in dup2 for even.\n");
//            exit(EXIT_FAILURE);
//        }

        printf("exec\n");

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for even.\n");
            exit(EXIT_FAILURE);
        }
    }

    *odd = fork();
    if (*odd == -1) {
        fprintf(stderr, "Fork for odd failed.\n");
        exit(EXIT_FAILURE);
    }

    if (*odd == 0) {
        printf("duplicate odd pipes\n");

        // need write pipe on one hand
        if(dup2(pipes[2][1], STDOUT_FILENO) == -1 || dup2(pipes[3][0], STDIN_FILENO) == -1) {
            fprintf(stderr, "Error in dup2 for odd.\n");
            exit(EXIT_FAILURE);
        }

        // dup2 pipes
//        if (dup2(pipes[1][0], STDIN_FILENO) == -1 || dup2(pipes[1][1], STDOUT_FILENO) == -1) {
//            fprintf(stderr, "Error in dup2 for odd.\n");
//            exit(EXIT_FAILURE);
//        }

        if (execlp(programmName, programmName, NULL) == -1) {
            fprintf(stderr, "Error in execlp for odd.\n");
            exit(EXIT_FAILURE);
        }
    }

    // close pipes
    for(int i = 0; i < 4; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}


static int readInput(double complex **numbers) {
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
 * @details Prints the output to the stdout if no outputFilename is specified
 * @param input Input to check
 * @return EXIT_FAILURE if input is not a (complex number)
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

// Function to perform FFT
void fft(float complex A[], int n, int precision, int pipe_even[2], int pipe_odd[2]) {
    if (n <= 1) {
        // Base case, nothing to do
        exit(EXIT_SUCCESS);
    }

    // Split the array into even and odd parts
    int n_half = n / 2;
    float complex even[n_half];
    float complex odd[n_half];
    for (int i = 0; i < n_half; i++) {
        even[i] = A[2 * i];
        odd[i] = A[2 * i + 1];
    }

    // Create two child processes
    pid_t child_even, child_odd;
    child_even = fork();
    if (child_even == -1) {
        fprintf(stderr, "Fork failed.\n");
        exit(EXIT_FAILURE);
    }

    if (child_even == 0) {
        // In the child_even process
        close(pipe_even[0]);
        close(pipe_odd[0]);
        close(pipe_odd[1]);

        // Perform FFT on the even part
        fft(even, n_half, precision, pipe_even, pipe_odd);

        // Send the result back to the parent through pipe
        write(pipe_even[1], even, n_half * sizeof(float complex));
        close(pipe_even[1]);

        exit(EXIT_SUCCESS);
    }

    // Parent process
    child_odd = fork();
    if (child_odd == -1) {
        fprintf(stderr, "Fork failed.\n");
        exit(EXIT_FAILURE);
    }

    if (child_odd == 0) {
        // In the child_odd process
        close(pipe_even[0]);
        close(pipe_even[1]);
        close(pipe_odd[0]);

        // Perform FFT on the odd part
        fft(odd, n_half, precision, pipe_even, pipe_odd);

        // Send the result back to the parent through pipe
        write(pipe_odd[1], odd, n_half * sizeof(float complex));
        close(pipe_odd[1]);

        exit(EXIT_SUCCESS);
    }

    // Parent process
    close(pipe_even[1]);
    close(pipe_odd[1]);

    // Wait for child processes to complete
    int status_even, status_odd;
    waitpid(child_even, &status_even, 0);
    waitpid(child_odd, &status_odd, 0);
    if (status_even != EXIT_SUCCESS || status_odd != EXIT_SUCCESS) {
        fprintf(stderr, "Child process failed.\n");
        exit(EXIT_FAILURE);
    }

    // Read the results from child processes
    read(pipe_even[0], even, n_half * sizeof(float complex));
    read(pipe_odd[0], odd, n_half * sizeof(float complex));
    close(pipe_even[0]);
    close(pipe_odd[0]);

    // Combine results using the butterfly operation
    for (int k = 0; k < n_half; k++) {
        float complex t = (cos((((-2 * PI) / n)) * k) + I * sin(((-2 * PI) / n) * k)) * odd[k];
        A[k] = even[k] + t;
        A[k + n_half] = even[k] - t;
    }
}