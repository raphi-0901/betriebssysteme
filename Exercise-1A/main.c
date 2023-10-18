#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

void toLowerCase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
}

void processFile(FILE *inputFile, const char *outputFilename, char *keyword, const bool caseSensitive) {
    FILE *outputFile = stdout; // Standardausgabe

    if (outputFilename) {
        outputFile = fopen(outputFilename, "w");
        if (outputFile == NULL) {
            fprintf(stderr, "Konnte die Ausgabedatei %s nicht öffnen.\n", outputFilename);
            fclose(inputFile);
            return;
        }
    }

    if (caseSensitive == false) {
        toLowerCase(keyword);
    }

    char line[1024];
    while (fgets(line, sizeof(line), inputFile)) {
        char *result = NULL;

        char updatedLine[strlen(line) + 1];
        strcpy(updatedLine, line);

        if (caseSensitive == false) {
            toLowerCase(updatedLine);
        }

        result = strstr(updatedLine, keyword);
        if (result != NULL) {
            fprintf(outputFile, "%s", line);
        }
    }

    fclose(inputFile);
    if (outputFile != stdout) {
        fclose(outputFile);
    }
}

int main(int argc, char *argv[]) {
    char *outputFilename = NULL;
    int opt;

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
        return EXIT_FAILURE;
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
                continue;
            }
            processFile(inputFile, outputFilename, keyword, caseSensitive);
        }
    }

    return EXIT_SUCCESS;
}
