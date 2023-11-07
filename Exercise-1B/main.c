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

/**
 * @brief Converts an input line to a double complex number.
 * @details Prints the output to the stdout if no outputFilename is specified
 * @param input Input to check
 * @return EXIT_FAILURE if input is not a (complex number)
 */
static double complex convertInputToNumber(char *input) {
    // Variablen für die Real- und Imaginärteile
    double real = 0.0, imag = 0.0;
    
    // Verarbeiten Sie den Eingabestring
    char *endptrReal;
    
    // Suchen Sie nach dem Realteil
    real = strtod(input, &endptrReal);
	
	if (endptrReal == input || real == 0.0) {
        printf("1- Konvertierung fehlgeschlagen.\n");
		exit(EXIT_FAILURE);
    }
    
    // Überspringen Sie Leerzeichen und eventuelles Minuszeichen
    while (*endptrReal == ' ') {
        endptrReal++;
    }

	// there is no further string after empty spaces after number
	if (*endptrReal == '\0' || *endptrReal == '\n') {
		double complex number = imag;
		return number;
    }
    

	char *endptrImag;
	// check if there is another number
    imag = strtod(endptrReal, &endptrImag);
	if (endptrReal == endptrImag || imag == 0.0) {
        printf("Konvertierung fehlgeschlagen.\n");
		exit(EXIT_FAILURE);
    }
	
    // check if there is a *i at the end
    if (*endptrImag != '*' || *(endptrImag+1) != 'i') {
		printf("Konvertierung fehlgeschlagen.\n");
		exit(EXIT_FAILURE);    
	}

	endptrImag += 2;

    // there are further chars after *i
	if (*endptrImag != '\0' && *endptrImag != '\n') {
		printf("Konvertierung fehlgeschlagen.\n");
		exit(EXIT_FAILURE);  
	}
    
    // Erzeugen Sie die komplexe Zahl
    double complex z = real + imag * I;
	// printf("%f %f*I\n", creal(z), cimag(z));
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

int main(int argc, char* argv[]) {
	// Parse command-line options if needed
	int precision = 6;  // Default precision
	if (argc > 1 && strcmp(argv[1], "-p") == 0) {
		precision = 3;
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	float complex A[100];  // Adjust the size as needed
	int initialCapacity = 2;
	float complex *numbers = (float complex *)calloc(initialCapacity, sizeof(float complex));

	if(numbers == NULL) {
		printf("Something went wrong allocating memory");
		exit(EXIT_FAILURE);
	}

	int n = 0;
	while((nread = getline(&line, &len, stdin)) != -1) {
		A[n] = 	convertInputToNumber(line);
		n++;

		if(n >= initialCapacity) {
			initialCapacity *= 2;
			numbers = realloc(numbers, initialCapacity * sizeof(float complex));

			if(numbers == NULL) {
				fprintf(stderr, "Something went wrong allocating memory\n");
				free(numbers);
				exit(EXIT_FAILURE);
			}
		}
	}
	free(line);

	// Just print the number
	if(n == 1) {
		printf("%.*f %.*f*i\n", precision, creal(A[0]), precision, cimag(A[0]));
		exit(EXIT_SUCCESS);
	}

	// Check for valid input
	if (n <= 0 || n % 2 != 0 || (n & (n - 1)) != 0) {
		fprintf(stderr, "Invalid input.\n");
		free(numbers);
		exit(EXIT_FAILURE);
	}

	// Create pipes for communication with child processes
	int pipe_even[2];
	int pipe_odd[2];

	if (pipe(pipe_even) == -1 || pipe(pipe_odd) == -1) {
		fprintf(stderr, "Pipe creation failed.\n");
		free(numbers);
		exit(EXIT_FAILURE);
	}

	// Perform FFT
	//fft(A, n, precision, pipe_even, pipe_odd);
	
	for(int i = 0; i < n; i++) {
		printf("%.*f %.*f*i\n", precision, creal(A[i]), precision, cimag(A[i]));
	}

	// Close pipes
	close(pipe_even[0]);
	close(pipe_even[1]);
	close(pipe_odd[0]);
	close(pipe_odd[1]);
	free(numbers);

	exit(EXIT_SUCCESS);
}

