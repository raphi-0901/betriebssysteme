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
#include "structs.h"
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h> /* For O_* constants */
static void usage();

#define SEM_READ "/sem_read"
#define SEM_WRITE "/sem_write"
#define SEM_MUTEX "/sem_mutex"

int delay = 0;
int limit = -1;
char *programName;

static volatile sig_atomic_t quit = false;
static void onSignal(int sig, siginfo_t *si, void *unused) { quit = true; }
static void initSignalHandler(void)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = onSignal;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction");
    }
    if (sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction");
    }
}

int main(int argc, char **argv)
{
    initSignalHandler();
    programName = argv[0];

    // set options
    int opt;
    int delayAlreadySet = 0;
    while ((opt = getopt(argc, argv, "n:w:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            // called more than once
            if (limit != -1)
            {
                usage();
            }

            limit = strtol(optarg, NULL, 10);
            break;
        case 'w':
            if (delayAlreadySet != 0)
            {
                usage();
            }

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

    Shm_t *myshm;
    int status = openOrCreateSharedMemory(&myshm);

    if (status == -1)
    {
        fprintf(stderr, "creation of shm failed");
        return 2;
    }

    // set values of shm
    myshm->writeIndex = 0;
    myshm->readIndex = 0;
    myshm->terminateGenerators = false;
    printf("Alive1!\n");
    printf("Alive2!\n");
    if (sem_init(&myshm->writeMutex, true, 1) == -1)
    {
        fprintf(stderr, "initialisation of semaphore failed");
        return 1;
    }
    if (sem_init(&myshm->numUsed, true, 0) == -1)
    {
        fprintf(stderr, "initialisation of semaphore failed");
        return 1;
    }
    if (sem_init(&myshm->numFree, true, MAX_DATA) == -1)
    {
        fprintf(stderr, "initialisation of semaphore failed");
        return 1;
    }

    int bestResult = INT_MAX;
    for (int i = 0; (i < limit || limit == -1) && !quit; i++)
    {
        if (sem_wait(&myshm->numUsed) == -1)
        {
            fprintf(stderr, "Failure in semaphore wait");
            exit(EXIT_FAILURE);
        }

        struct EdgeDTO *cancelledEdges = &myshm->buffer[myshm->readIndex];
        // fprintf(stderr, "Got %d edges\n", cancelledEdges->edgeCount);
        if (cancelledEdges->edgeCount < bestResult)
        {
            bestResult = cancelledEdges->edgeCount;
            if (bestResult == 0)
            {
                fprintf(stdout, "The graph is 3-colorable!\n");
                // make sure to clean shared memory and semaphores
                exit(EXIT_SUCCESS);
            }
            else
            {
                // todo print all edges in cancelledEdges result
                fprintf(stderr, "The graph is 3-colorable by removing %d edges: ", bestResult);
                for (size_t i = 0; i < cancelledEdges->edgeCount; i++)
                {
                    printf("%d-%d ", cancelledEdges->results[i].vertex1.id, cancelledEdges->results[i].vertex1.id);
                }
                printf("\n");
            }
        }

        myshm->readIndex = (myshm->readIndex + 1) % MAX_DATA;
        if (sem_post(&myshm->numFree) == -1)
        {
            fprintf(stderr, "Failure in semaphore wait");
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges\n", bestResult);

    printf("Terminating generators...\n");
    myshm->terminateGenerators = true;
    // for (size_t i = 0; i < shmp->numGenerators; i++)
    // {
    //     if (sem_post(&shmp->numFree) == -1)
    //     {
    //         perror("sem_post - error occured while freeing the waiting generators");
    //     }
    // }
    // size_t len = 0;
    // char *line = NULL;
    // while (getline(&line, &len, stdin) != -1)
    // {
    //     printf("line: %s", line);
    // }

    if (sem_destroy(&(myshm->writeMutex)) == -1)
    {
        fprintf(stderr, "Failure in semaphore destroy");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&(myshm->numUsed)) == -1)
    {
        fprintf(stderr, "Failure in semaphore destroy");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&(myshm->numFree)) == -1)
    {
        fprintf(stderr, "Failure in semaphore destroy");
        exit(EXIT_FAILURE);
    }

    closeSharedMemory(myshm);
}

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage()
{
    fprintf(stderr, "Usage:\t%s [-n limit] [-w delay]\n", programName);
    exit(EXIT_FAILURE);
}
