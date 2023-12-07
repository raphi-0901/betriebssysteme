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
#include <errno.h>

static void usage();

char *name;

static char *colorToString(enum Color color);

static struct Edge convertInputToEdge(char *input);

#define SEM_READ "/sem_read"
#define SEM_WRITE "/sem_write"
#define SEM_MUTEX "/sem_mutex"

int main(int argc, char **argv)
{
    name = argv[0];

    int edgeCount = argc - 1;
    if (edgeCount == 0)
    {
        usage();
    }

    struct Edge *edges = (struct Edge *)malloc(edgeCount * sizeof(struct Edge));
    for (int i = 0; i < edgeCount; i++)
    {
        edges[i] = convertInputToEdge(argv[i + 1]);
    }

    // Initialisieren des Zufallszahlengenerators mit der aktuellen Zeit
    srand(time(NULL));

    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1)
    {
        fprintf(stderr, "Failure in semaphore wait\n");
        exit(EXIT_FAILURE);
    }
    Shm_t *myshm =
        mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (myshm == MAP_FAILED)
    {
        fprintf(stderr, "Failure in semaphore wait\n");
        exit(EXIT_FAILURE);
    }

    close(fd);

    int writePos = 0;

    // generate while supervisor does not end
    while (!myshm->terminateGenerators)
    {
        // assign random color to vertices
        for (int i = 0; i < edgeCount; i++)
        {
            int randomColor1 = rand() % 3;
            int randomColor2 = rand() % 3;
            edges[i].vertex1.color = randomColor1;
            edges[i].vertex2.color = randomColor2;

            //            printf("%d - %d\t%s - %s\n", edges[i].vertex1.id, edges[i].vertex2.id, colorToString(edges[i].vertex1.color),
            //                   colorToString(edges[i].vertex2.color));
        }

        // check for a solution
        EdgeDTO edgeDTO;
        int cancelledEdgeCounter = 0;
        for (int i = 0; i < edgeCount; i++)
        {
            Edge edge = edges[i];
            if (edge.vertex1.color == edge.vertex2.color)
            {
                edgeDTO.results[cancelledEdgeCounter] = edge;
                cancelledEdgeCounter++;
            }
        }
        edgeDTO.edgeCount = cancelledEdgeCounter;

        // probieren schlechte ergebnisse gar nicht zu schreiben
        if (cancelledEdgeCounter > 8)
        {
            continue;
        }

        // wait for empty space on buffer
        if ((sem_wait(&myshm->numFree) == -1) && (errno != EINTR))
        {
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }

        if (myshm->terminateGenerators)
        {
            // supervisor called sem_post() to free generators for shutdown
            break;
        }

        // wait for exclusive write access on buffer
        if ((sem_wait(&myshm->writeMutex) == -1) && (errno != EINTR))
        {
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }

        // write solution to buffer and update writeIndex
        myshm->buffer[myshm->writeIndex] = edgeDTO;
        myshm->writeIndex = (myshm->writeIndex + 1) % MAX_DATA;

        // free exclusive access
        if (sem_post(&myshm->writeMutex) == -1)
        {
            fprintf(stderr, "Failure in semaphore post\n");
            exit(EXIT_FAILURE);
        }

        // increase used space on buffer
        if (sem_post(&myshm->numUsed) == -1)
        {
            fprintf(stderr, "Failure in semaphore post\n");
            exit(EXIT_FAILURE);
        }

        // break if we found a perfect solution
        if (cancelledEdgeCounter == 0)
        {
            break;
        }
    }

    free(edges);
    closeSharedMemory(myshm, true);
}

static char *colorToString(enum Color color)
{
    char *colorName = "";
    switch (color)
    {
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

static struct Edge convertInputToEdge(char *input)
{
    regex_t regex;
    int reti;

    char *pattern = "^[0-9]+-[0-9]+$";

    // Kompiliere den regulären Ausdruck
    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti)
    {
        fprintf(stderr, "Fehler beim Kompilieren des regulären Ausdrucks\n");
        regfree(&regex);
        exit(1);
    }

    reti = regexec(&regex, input, 0, NULL, 0);
    if (reti == REG_NOMATCH)
    {
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
void usage()
{
    fprintf(stderr, "Usage:\t%s EDGE1\n", name);
    fprintf(stderr, "EXAMPLE:\t%s 0-1 0-2 0-3 1-2 1-3 2-3\n", name);
    exit(EXIT_FAILURE);
}