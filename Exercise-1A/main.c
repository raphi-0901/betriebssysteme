#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

void processFile(const char *inputFilename, const char *outputFilename, const char *keyword, const bool caseSensitive) {
    FILE *inputFile = fopen(inputFilename, "r");
    if (inputFile == NULL) {
        fprintf(stderr, "Konnte die Eingabedatei %s nicht öffnen.\n", inputFilename);
        return;
    }

    FILE *outputFile = stdout; // Standardausgabe

    if (outputFilename) {
        outputFile = fopen(outputFilename, "w");
        if (outputFile == NULL) {
            fprintf(stderr, "Konnte die Ausgabedatei %s nicht öffnen.\n", outputFilename);
            fclose(inputFile);
            return;
        }
    }

    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), inputFile)) {
        char *result = strstr(line, keyword);

        if (result != NULL) {
            fprintf(outputFile, "%s:%d: %s\n", inputFilename, i, line);
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

    bool caseSensitive = false;
    while ((opt = getopt(argc, argv, "io:")) != -1) {
        switch (opt) {
            case 'i':
                caseSensitive = true;
                break;
            case 'o':
                outputFilename = optarg;
                break;
            default:
                break;
        }
    }

    if(optind >= argc) {
        return EXIT_FAILURE;
    }

    char *keyword = argv[optind];
    optind++;
    for (; optind < argc; optind++){
        processFile(argv[optind], outputFilename, keyword, caseSensitive);
    }

    return EXIT_SUCCESS;
}
