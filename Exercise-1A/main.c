/**
 * @file main.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 10.11.2023
 *
 * @brief Source file of mygrep programm.
 *
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

char* name;

static void processFile(FILE *inputFile, const char *outputFilename, char *keyword, const bool caseSensitive);
static void usage(void);

/**
 * Program entry point.
 * @brief Reads all arguments and processes all specified files one by one.
 * @details If there is no input files specified, the programm uses stdin as inputFile.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[]) {
    name = argv[0];
    char *outputFilename = NULL;
    int opt;

    if(argc < 2) {
        usage();
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
                usage();
                break;
        }
    }

    if (optind >= argc) {
        usage();
    }

    char *keyword = argv[optind];
    optind++;
    if (optind >= argc) {
        processFile(stdin, outputFilename, keyword, caseSensitive);
    } else {
        for (; optind < argc; optind++) {
            FILE *inputFile = fopen(argv[optind], "r");
            if (inputFile == NULL) {
                fprintf(stderr, "Cannot open input file: %s.\n", argv[optind]);
                exit(EXIT_FAILURE);
            }
            processFile(inputFile, outputFilename, keyword, caseSensitive);
        }
    }

    return EXIT_SUCCESS;
}

/**
 * Process a file.
 * @brief Process one input file´s input and prints all the found lines.
 * @details Prints the output to the stdout if no outputFilename is specified
 * @param inputFile File stream to input file
 * @param outputFilename relative path of outputfile, if null stdout is used
 * @param keyword Keyword to search for
 * @param caseSensitive CaseSensitive parameter
 * @return EXIT_FAILURE if there is a problem with the output corresponding with the specified outputFilename
 */
static void processFile(FILE *inputFile, const char *outputFilename, char *keyword, const bool caseSensitive) {
	FILE *outputFile = stdout;
	if (outputFilename) {
		outputFile = fopen(outputFilename, "w");
		if (outputFile == NULL) {
			fprintf(stderr, "Cannot open file: %s.\n", outputFilename);
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

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage(void) {
	fprintf(stderr, "Usage:\t%s [-i] [-o outputfile] [inputfile...]\n", name);
	exit(EXIT_FAILURE);
}