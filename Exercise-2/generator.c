/**
 * @file supervisor.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Source file of supervisor programm of 3-coloring.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "shared_memory.h"
#include <regex.h>
#include <string.h>

#define SHM_NAME "/myshm"
#define MAX_DATA (50)

static void usage(char *name);

static void handle_signal(int signal);

volatile sig_atomic_t quit = 0;

int delay = 0;
int limit = -1;
char *name;


struct Vertex {
    int id;
    int color;
};

struct Edge {
    struct Vertex vertex1;
    struct Vertex vertex2;
};

static struct Edge convertInputToEdge(char *input);

int main(int argc, char **argv) {
    // Durchlaufen Sie die Argumente und verarbeiten Sie sie
    struct Edge *edges = (struct Edge *) malloc((argc - 1) * sizeof(struct Edge));

    char *token = strtok("0-1", "-");

    while (token != NULL) {
        // token enthält das aufgeteilte Teil des Strings
        printf("Token: %s\n", token);

        // Rufen Sie strtok erneut auf, um das nächste Token zu erhalten
        token = strtok(NULL, "-");
    }

    for (int i = 1; i < argc; i++) {
        edges[i] = convertInputToEdge(argv[i]);
        // Jetzt können Sie das Argument weiter analysieren und verwenden
    }


    for (int i = 0; i < argc - 1; i++) {
        printf("edge: %d - %d\n", edges[i].vertex1.id, edges[i].vertex2.id);
    }
}

static struct Edge convertInputToEdge(char *input) {
    regex_t regex;
    int reti;

    char *pattern = "^[0-9]+-[0-9]+$";

    // Kompiliere den regulären Ausdruck
    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Fehler beim Kompilieren des regulären Ausdrucks\n");
        regfree(&regex);
        exit(1);
    }

    reti = regexec(&regex, input, 0, NULL, 0);
    if (reti == REG_NOMATCH) {
        {
            fprintf(stderr, "Input does not match us.\n");
            regfree(&regex);
            usage("");
        }
    }

    struct Vertex vertex1;
    struct Vertex vertex2;
    vertex1.id = strtol(strtok(input, "-"), NULL, 10);
    vertex1.color = -1;
    vertex2.id = strtol(strtok(NULL, "-"), NULL, 10);
    vertex2.color = -1;
    struct Edge edge;
    edge.vertex1 = vertex1;
    edge.vertex2 = vertex2;
    regfree(&regex);
    return edge;
}

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage(char *name) {
    fprintf(stderr, "Usage:\t%s [-n limit] [-w delay]\n", name);
    exit(EXIT_FAILURE);
}

static void handle_signal(int signal) { quit = 1; }
