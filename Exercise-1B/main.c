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
static double convertNumber(char* str);

static double convertNumber(char* str) {
    if (str == NULL || *str == '\0') {
		exit(EXIT_FAILURE);
    }

	errno = 0;
	char *endPointer;


	// Regex-Muster für die gewünschten Zeichenfolgen
	const char *pattern = "^[ \t]*[-+]?[0-9]+(\\.[0-9]+)?[ \t]*$"; // Regex-Muster für positive und negative Zahlen
    regex_t regex;
    int ret;

    ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Fehler beim Kompilieren des regulären Ausdrucks\n");
        return 1;
    }

	printf("%s",str);


	if (regexec(&regex, str, 0, NULL, 0) == REG_NOMATCH) {
		printf("No match: %s\n", str);
	} else {
		printf("Match: %s\n", str);
	}

    regfree(&regex);

    double number = strtof(str, &endPointer); // Attempt to convert the string to a long integer



	if(errno != 0) {
		fprintf(stderr, "Error occured");
		exit(EXIT_FAILURE);
	}

	printf("s number: %f\n", number);

	printf("converted number: %f\n", number);

    return number; // Successfully parsed as a number
}

/**
 * @brief Converts an input line to a double complex number.
 * @details Prints the output to the stdout if no outputFilename is specified
 * @param input Input to check
 * @return EXIT_FAILURE if input is not a (complex number)
 */
static double complex convertInputToNumber(char *input) {
    // const char *patterns[] = {"-?\\d+\\.\\d+\\s\\d+\\*i", "\\d+\\s\\d+\\*i", "\\d+"};
    char *token;
    double complex number = 22;

    // split string into parts.. first part should match any number
    token = strtok(input, " ");
	if(token == NULL) {
		// error - no input
	}

	number = convertNumber(token);

	// get second part of string (if it exists + check for any number and a ending i)
    token = strtok(NULL, " ");
	if(token != NULL) {

		//check with regex
	}

	
	printf("%s", token);
	printf("%s", token);

	// start converting number
    // double complex number = strtof(input, &input);
	return number;
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

