#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void processFile(FILE *file, char *name);
char *processLine(char *input, char *outputPointer);

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        processFile(stdin, "stdin");
    }

    int i = 1;
    while (argv[i] != NULL)
    {
        printf("%s\n", argv[i]);
        FILE *stream = fopen(argv[i], "r");
        if (stream == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        processFile(stream, argv[i]);
        fclose(stream);

        i++;
    }

    return 0;
}

void processFile(FILE *file, char *name)
{
    char *line = NULL;
    size_t len = 0;
    char *suffix = ".comp";

    char *outputFilename = malloc(strlen(name) + strlen(suffix) + 1);
    if (outputFilename == NULL)
    {
        fprintf(stderr, "Allocating memory failed in processFile");
        exit(EXIT_FAILURE);
    }

    // format filename to filename.comp
    sprintf(outputFilename, "%s%s", name, suffix);
    FILE *outputFile = fopen(outputFilename, "w");

    if (outputFile == NULL)
    {
        fprintf(stderr, "Open outputfile was not possible");
        exit(EXIT_FAILURE);
    }

    int sizeOfOld = 0;
    int sizeOfNew = 0;
    while ((getline(&line, &len, file)) != -1)
    {
        char *newLine = malloc(strlen(line) * 2 + 1);
        processLine(line, newLine);
        // fprintf(stdout, "%s", newLine);
        fprintf(outputFile, "%s", newLine);
        sizeOfOld += strlen(line);
        sizeOfNew += strlen(newLine);
    }

    fprintf(stderr, "old: %d\nnew: %d\n", sizeOfOld, sizeOfNew);

    free(line);
}

char *processLine(char *input, char *outputPointer)
{
    int counterLastChar = 0;
    char lastChar = '\0';
    char partOfLine[1 + sizeof(int)];

    for (int i = 0; input[i] != '\0' && input[i] != '\n'; ++i)
    {
        if (input[i] != lastChar || lastChar == '\0')
        {
            sprintf(partOfLine, "%c%d", lastChar, counterLastChar);
            strcat(outputPointer, partOfLine);
            printf("%s", outputPointer);
            counterLastChar = 0;
            lastChar = input[i];
        }
        counterLastChar++;
        // printf("%c\n", input[i]); // Ausgabe jedes einzelnen Zeichens
    }

    // add last sign
    if (lastChar != '\0')
    {
        sprintf(partOfLine, "%c%d\n", lastChar, counterLastChar);
        strcat(outputPointer, partOfLine);
    }

    return "";
}