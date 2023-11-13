/**
 * @file supervisor.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Source file of generator programm of 3-coloring.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include "shared_memory.h"
#include <regex.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

static void usage();

char *name;

enum Color {
    UNASSIGNED = -1,
    RED = 0,
    GREEN = 1,
    BLUE = 2,
};

struct Vertex {
    int id;
    enum Color color;
};

struct Edge {
    struct Vertex vertex1;
    struct Vertex vertex2;
};

static char *colorToString(enum Color color);

static struct Edge convertInputToEdge(char *input);

#define SEM_READ "/sem_read"
#define SEM_WRITE "/sem_write"
#define SEM_MUTEX "/sem_mutex"

int main(int argc, char **argv) {
    name = argv[0];
    struct myshm *myshm;
    openOrCreateSharedMemory(&myshm);
    sem_t *semRead = sem_open(SEM_READ, 0);
    sem_t *semWrite = sem_open(SEM_WRITE, 0);
    //sem_t *semMutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL);

    int edgeCount = argc - 1;
    struct Edge *edges = (struct Edge *) malloc(edgeCount * sizeof(struct Edge));

    for (int i = 0; i < edgeCount; i++) {
        edges[i] = convertInputToEdge(argv[i + 1]);
    }

    // Initialisieren des Zufallszahlengenerators mit der aktuellen Zeit
    srand(time(NULL));

    int writePos = 0;
    while (1) {
        // assign random color to vertices
        for (int i = 0; i < edgeCount; i++) {
            int randomColor1 = rand() % 3;
            int randomColor2 = rand() % 3;
            edges[i].vertex1.color = randomColor1;
            edges[i].vertex2.color = randomColor2;

//            printf("%d - %d\t%s - %s\n", edges[i].vertex1.id, edges[i].vertex2.id, colorToString(edges[i].vertex1.color),
//                   colorToString(edges[i].vertex2.color));

        }

        int cancelledEdges = 0;
        for (int i = 0; i < edgeCount; i++) {
            struct Edge edge = edges[i];
            if (edge.vertex1.color == edge.vertex2.color) {
                cancelledEdges++;
            }
        }
        //fprintf(stdout, "3 colorable with removing %d edges\n", cancelledEdges);

        sem_wait(semWrite);
        myshm->results[writePos] = cancelledEdges;
        writePos = (writePos + 1) % MAX_DATA;
        sem_post(semRead);

        if(cancelledEdges == 0) {
            break;
        }
    }

    // write to sharedMemory
    closeSharedMemory(myshm);
}

static char *colorToString(enum Color color) {
    char *colorName = "";
    switch (color) {
        case RED:
            colorName = "RED";
            break;
        case GREEN:
            colorName = "GREEN";
            break;
        case BLUE:
            colorName = "BLUE";
            break;
        default:
            colorName = "UNASSIGNED";
            break;
    }
    return colorName;
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
    vertex1.color = UNASSIGNED;
    vertex2.id = strtol(strtok(NULL, "-"), NULL, 10);
    vertex2.color = UNASSIGNED;
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
void usage() {
    fprintf(stderr, "Usage:\t%s EDGE1\n", name);
    fprintf(stderr, "EXAMPLE:\t%s 0-1 0-2 0-3 1-2 1-3 2-3\n", name);
    exit(EXIT_FAILURE);
}