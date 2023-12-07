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
#include <regex.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>
#include "structs.h"
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

static void usage();

char *name;

static struct Edge convertInputToEdge(char *input);

int main(int argc, char **argv)
{
    name = argv[0];

    int edgeCount = argc - 1;
    if (edgeCount == 0)
    {
        usage();
    }

    struct Edge edges[edgeCount];
    for (int i = 0; i < edgeCount; i++)
    {
        edges[i] = convertInputToEdge(argv[i + 1]);
    }

    Vertex vertices[edgeCount * 2]; // cannot be more than twice the edgeCount

    // add all vertices to vertices array
    int vertexCounter = 0;
    for (int i = 0; i < edgeCount; i++)
    {
        Vertex vertex1 = edges[i].vertex1;
        Vertex vertex2 = edges[i].vertex2;

        bool foundVertex1 = false;
        bool foundVertex2 = false;
        for (int j = 0; j < vertexCounter; j++)
        {
            if (foundVertex1 && foundVertex2)
            {
                break;
            }

            Vertex compareVertex = vertices[j];

            if (compareVertex.id == vertex1.id)
            {
                foundVertex1 = true;
            }

            if (compareVertex.id == vertex2.id)
            {
                foundVertex2 = true;
            }
        }

        if (!foundVertex1)
        {
            vertices[vertexCounter++] = vertex1;
        }

        if (!foundVertex2)
        {
            vertices[vertexCounter++] = vertex2;
        }
    }

    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1)
    {
        fprintf(stderr, "Failure while opening shared memory\n");
        exit(EXIT_FAILURE);
    }

    Shm_t *myshm =
        mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (myshm == MAP_FAILED)
    {
        fprintf(stderr, "Failure while mapping shared memory\n");
        exit(EXIT_FAILURE);
    }

    // open semaphores
    sem_t *writeMutex = sem_open(SEM_WRITE_MUTEX, 0);
    sem_t *numUsed = sem_open(SEM_NUM_USED, 0);
    sem_t *numFree = sem_open(SEM_NUM_FREE, 0);

    close(fd);

    // Initialisieren des Zufallszahlengenerators mit der aktuellen Zeit
    srand(time(NULL));

    // generate while supervisor does not end
    while (!myshm->terminateGenerators)
    {
        // assign random colors
        // color nodeArray
        for (size_t i = 0; i < vertexCounter; i++)
        {
            vertices[i].color = rand() % 3;
        }

        for (size_t i = 0; i < edgeCount; i++)
        {
            for (size_t j = 0; j < vertexCounter; j++)
            {
                if (vertices[j].id == edges[i].vertex1.id)
                {
                    edges[i].vertex1.color = vertices[j].color;
                }

                if (vertices[j].id == edges[i].vertex2.id)
                {
                    edges[i].vertex2.color = vertices[j].color;
                }
            }
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
        if ((sem_wait(numFree) == -1) && (errno != EINTR))
        {
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }

        if (myshm->terminateGenerators)
        {
            // supervisor wants to free generators for shutdown
            break;
        }

        // wait for exclusive write access on buffer
        if ((sem_wait(writeMutex) == -1) && (errno != EINTR))
        {
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }

        // write solution to buffer and update writeIndex
        myshm->buffer[myshm->writeIndex] = edgeDTO;
        myshm->writeIndex = (myshm->writeIndex + 1) % MAX_DATA;

        // free exclusive access
        if (sem_post(writeMutex) == -1)
        {
            fprintf(stderr, "Failure in semaphore post\n");
            exit(EXIT_FAILURE);
        }

        // increase used space on buffer
        if (sem_post(numUsed) == -1)
        {
            fprintf(stderr, "Failure in semaphore post\n");
            exit(EXIT_FAILURE);
        }
    }

    if (sem_close(writeMutex) == -1 || sem_close(numUsed) == -1 || sem_close(numFree) == -1)
    {
        fprintf(stderr, "Failure in semaphore close");
        exit(EXIT_FAILURE);
    }

    // unmap shared memory:
    if (munmap(myshm, sizeof(*myshm)) == -1)
    {
        fprintf(stderr, "error in munmap\n");
    }
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