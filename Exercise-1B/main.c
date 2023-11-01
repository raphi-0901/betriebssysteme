#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#define EXIT_FAILURE 1
//#define EXIT_SUCCESS 0
#define PI 3.141592654

//void fft(double A[], int n);

// Function to perform FFT
void fft(double A[], int n, int precision, int pipe_even[2], int pipe_odd[2]) {
    if (n <= 1) {
        // Base case, nothing to do
        return;
    }

    // Split the array into even and odd parts
    int n_half = n / 2;
    double even[n_half];
    double odd[n_half];
    for (int i = 0; i < n_half; i++) {
        even[i] = A[2 * i];
        odd[i] = A[2 * i + 1];
    }

    // Create two child processes
    pid_t child_even, child_odd;
    child_even = fork();
    if (child_even == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_even == 0) {
        // In the child_even process
        close(pipe_even[0]);
        close(pipe_odd[0]);
        close(pipe_odd[1]);

        // Perform FFT on the even part
        fft(even, n_half, precision, NULL, NULL);

        // Send the result back to the parent through pipe
        write(pipe_even[1], even, n_half * sizeof(float complex));
        close(pipe_even[1]);

        exit(EXIT_SUCCESS);
    }

    // Parent process
    child_odd = fork();
    if (child_odd == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_odd == 0) {
        // In the child_odd process
        close(pipe_even[0]);
        close(pipe_even[1]);
        close(pipe_odd[0]);

        // Perform FFT on the odd part
        fft(odd, n_half, precision, NULL, NULL);

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
        ////float complex t = cexp(-I * 2.0 * PI * k / n) * odd[k];
        //A[k] = even[k] + t;
        //A[k + n_half] = even[k] - t;
    }
}

int main(int argc, char* argv[]) {
    // Parse command-line options if needed
    int precision = 6;  // Default precision
    if (argc > 1 && strcmp(argv[1], "-p") == 0) {
        precision = 3;
    }

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
    double A[100];  // Adjust the size as needed
	int n = 0;

	while((nread = getline(&line, &len, stdin)) != -1) {
		fwrite(line, nread, 1, stdout);
		A[n] = atof(line);
		n++;
	}

	printf("line: %s", line);
	free(line);

	for(int i = 0; i < n; i++) {
		printf("number %d: %f\n", i, A[i]);
	}

    // Check for valid input
    if (n <= 0 || n % 2 != 0) {
        fprintf(stderr, "Invalid input.\n");
        exit(EXIT_FAILURE);
    }

    // Create pipes for communication with child processes
    int pipe_even[2];
    int pipe_odd[2];

    if (pipe(pipe_even) == -1 || pipe(pipe_odd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Perform FFT
    fft(A, n, precision, pipe_even, pipe_odd);

    // Close pipes
    close(pipe_even[0]);
    close(pipe_even[1]);
    close(pipe_odd[0]);
    close(pipe_odd[1]);

    exit(EXIT_SUCCESS);
}


