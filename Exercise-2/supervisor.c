/**
 * @file supervisor.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Source file of supervisor programm of 3-coloring.
 *
 **/

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "shared_memory.h"
#include <semaphore.h>
#include <fcntl.h> /* For O_* constants */
static void usage(char *name);

#define SEM_READ "/sem_read"
#define SEM_WRITE "/sem_write"
#define SEM_MUTEX "/sem_mutex"

int delay = 0;
int limit = -1;
char *name;

int main(int argc, char **argv) {
    struct myshm *myshm;
    openOrCreateSharedMemory(&myshm);

    sem_t *semRead = sem_open(SEM_READ, O_CREAT | O_EXCL, 0600, 0);
    sem_t *semWrite = sem_open(SEM_WRITE, O_CREAT | O_EXCL, 0600, MAX_DATA);
    sem_t *semMutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, 0600, 1);

    int opt;
    while ((opt = getopt(argc, argv, "n:w:")) != -1) {
        switch (opt) {
            case 'n':
                limit = strtol(optarg, NULL, 10);
                break;
            case 'w':
                delay = strtol(optarg, NULL, 10);
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    printf("Warten auf %d Sekunden...\n", delay);
    sleep(delay);
    printf("Wartezeit abgeschlossen!\n");

    unsigned int bestResult = INT_MAX;
    int readPos = 0;
    for (int i = 0; i < limit || limit == -1; i++) {
        sem_wait(semRead);
        unsigned int *result = &myshm->results[readPos];
        if(*result < bestResult) {
            bestResult = *result;

            if(bestResult == 0) {
                fprintf(stdout, "The graph is 3-colorable!\n");
                // make sure to clean shared memory and semaphores
                exit(EXIT_SUCCESS);
            } else {
                fprintf(stderr, "The graph is 3-colorable by removing %d edges\n", bestResult);
            }
        }



        readPos = (readPos + 1) % MAX_DATA;
        sem_post(semWrite);
    }

    fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges\n", bestResult);

    size_t len = 0;
    char *line = NULL;
    while (getline(&line, &len, stdin) != -1) {
        printf("line: %s", line);
    }

    closeSharedMemory(myshm);
    sem_close(semRead);
    sem_close(semWrite);
    sem_close(semMutex);
    sem_unlink(SEM_READ);
    sem_unlink(SEM_WRITE);
    sem_unlink(SEM_MUTEX);
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
