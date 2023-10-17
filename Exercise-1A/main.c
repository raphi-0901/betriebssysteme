#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void processFile(const char *inputFilename, const char *outputFilename) {
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

    char line[256]; // Annahme: Zeilen sind nicht länger als 255 Zeichen
    while (fgets(line, sizeof(line), inputFile)) {
        // Hier kannst du die Zeilen verarbeiten und in die Ausgabedatei schreiben
        fprintf(outputFile, "%s", line);
    }

    fclose(inputFile);

    if (outputFile != stdout) {
        fclose(outputFile);
    }
}

int main(int argc, char *argv[]) {
    char *outputFilename = NULL;
    int opt;
    
    while ((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch (opt) {
            case 'i':
                // Verarbeite die Eingabedatei und verwende outputFilename
                processFile(optarg, outputFilename);
                break;
            case 'o':
                outputFilename = optarg;
                break;
            default:
                fprintf(stderr, "Verwendung: %s -i Datei -o Ausgabedatei\n", argv[0]);
                return 1;
        }
    }

    return 0;
}
