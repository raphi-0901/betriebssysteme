#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

//TODO: makefile!! longline, error messages, testcase 10, usage messages
void processFile(FILE *inputFile, const char *outputFilename, char *keyword, const bool caseSensitive) {
	FILE *outputFile = stdout; // Standardausgabe
	if (outputFilename) {
		outputFile = fopen(outputFilename, "w");
		if (outputFile == NULL) {
			fprintf(stderr, "Konnte die Ausgabedatei %s nicht öffnen.\n", outputFilename);
			fclose(inputFile);
			exit(EXIT_FAILURE);
		}
	}
	size_t len = 0;
	char *line = NULL;
	while (getline(&line, &len, inputFile) != -1) {
		char *result = NULL;
		if (caseSensitive == false) {
			result = strcasestr(line, keyword);
		} else {
			result = strstr(line, keyword);
		}

		if (result != NULL) {
			fprintf(outputFile, "%s", line);
		}
	}

	free(line);
	fclose(inputFile);
	if (outputFile != stdout) {
		fclose(outputFile);
	}
}

int main(int argc, char *argv[]) {
	char *outputFilename = NULL;
	int opt;

	if(argc < 2) {
		printf("Description: This program requires an argument.\n");
		exit(EXIT_FAILURE);
	}

	bool caseSensitive = true;
	while ((opt = getopt(argc, argv, "io:")) != -1) {
		switch (opt) {
			case 'i':
				caseSensitive = false;
				break;
			case 'o':
				outputFilename = optarg;
				break;
			default:
				break;
		}
	}

	if (optind >= argc) {
		exit(EXIT_FAILURE);
	}

	char *keyword = argv[optind];
	optind++;
	if (optind >= argc) {
		processFile(stdin, outputFilename, keyword, caseSensitive);
	} else {
		for (; optind < argc; optind++) {
			FILE *inputFile = fopen(argv[optind], "r");
			if (inputFile == NULL) {
				fprintf(stderr, "Konnte die Eingabedatei %s nicht öffnen.\n", argv[optind]);
				exit(EXIT_FAILURE);
			}
			processFile(inputFile, outputFilename, keyword, caseSensitive);
		}
	}

	return EXIT_SUCCESS;
}
