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

static void usage(char *name);

int delay = 0;
int limit = -1;
char *name;

int main(int argc, char **argv) {
    struct myshm *myshm;
    openOrCreateSharedMemory(&myshm);

    printf("%d", myshm->results[0]);
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

    int bestResult = INT_MAX;
    for (int i = 0; i < limit; i++) {
        // if(bestResult > result) {
        //     bestResult = result;
        // }
        // read from buffer
        // if(result == 0) {
        //     fprintf(stdout, "The graph is 3-colorable!");
        // // make sure to clean shared memory and semaphores
        //     exit(EXIT_SUCCESS);
        // }
    }

    fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges\n", bestResult);

    size_t len = 0;
    char *line = NULL;
    while (getline(&line, &len, stdin) != -1) {
        printf("line: %s", line);
    }

    closeSharedMemory(&myshm);
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
