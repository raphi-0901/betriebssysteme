/**
 * @file supervisor.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Source file of supervisor program of 3-coloring.
 *
 **/

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "structs.h"
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>

static void usage();

int delay = 0;
int limit = -1;
char *programName;

static volatile sig_atomic_t quit = false;
static void onSignal(int sig, siginfo_t *si, void *unused) { quit = true; }

/**
 * Signal Handler.
 * @brief Intercepts the signals
 */
static void initSignalHandler(void)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = onSignal;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction\n");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction\n");
    }
    if (sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction\n");
    }
}

/**
 * Program entry point.
 * @brief Waits for generators to write solutions into the shared memory.
 * @param argc The argument counter.
 * @param argv The optional arguments.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
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

    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
    {
        fprintf(stderr, "initialization of shared memory failed\n");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd, sizeof(Shm_t)) == -1)
    {
        fprintf(stderr, "initialization of shared memory failed\n");
        exit(EXIT_FAILURE);
    }
    Shm_t *myshm =
        mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (myshm == MAP_FAILED)
    {
        fprintf(stderr, "initialization of shared memory failed\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1)
    {
        fprintf(stderr, "Failure while closing fd\n");
        exit(EXIT_FAILURE);
    }

    // set values of shared memory
    myshm->writeIndex = 0;
    myshm->readIndex = 0;
    myshm->terminateGenerators = false;

    sem_t *writeMutex = sem_open(SEM_WRITE_MUTEX, O_CREAT | O_EXCL, 0600, 1);
    sem_t *numUsed = sem_open(SEM_NUM_USED, O_CREAT | O_EXCL, 0600, 0);
    sem_t *numFree = sem_open(SEM_NUM_FREE, O_CREAT | O_EXCL, 0600, MAX_DATA);

    if (writeMutex == SEM_FAILED || numUsed == SEM_FAILED || numFree == SEM_FAILED)
    {
        fprintf(stderr, "initialization of semaphores failed\n");
        exit(EXIT_FAILURE);
    }

    sleep(delay);

    int bestResult = INT_MAX;
    for (int i = 0; (i < limit || limit == -1) && !quit; i++)
    {
        if (sem_wait(numUsed) == -1)
        {
            if (errno == EINTR)
            {
                fprintf(stderr, "Quit waiting\n");
                break;
            }
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }

        struct EdgeDTO *cancelledEdges = &myshm->buffer[myshm->readIndex];
        if (cancelledEdges->edgeCount < bestResult)
        {
            bestResult = cancelledEdges->edgeCount;
            if (bestResult == 0)
            {
                break;
            }
            // else
            // {
            //     fprintf(stderr, "The graph is 3-colorable by removing %d edges: ", bestResult);
            //     for (size_t i = 0; i < cancelledEdges->edgeCount; i++)
            //     {
            //         printf("%d-%d ", cancelledEdges->results[i].vertex1.id, cancelledEdges->results[i].vertex2.id);
            //     }
            //     printf("\n");
            // }
        }

        myshm->readIndex = (myshm->readIndex + 1) % MAX_DATA;
        if (sem_post(numFree) == -1)
        {
            fprintf(stderr, "Failure in semaphore wait\n");
            exit(EXIT_FAILURE);
        }
    }

    if (bestResult == 0)
    {
        fprintf(stdout, "The graph is 3-colorable!\n");
    }
    else
    {
        fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges.\n", bestResult);
    }

    myshm->terminateGenerators = true;
    for (size_t i = 0; i < myshm->countGenerator; i++)
    {
        if (sem_post(numFree) == -1)
        {
            fprintf(stderr, "error in semaphore post while terminating generators.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (sem_close(writeMutex) == -1 || sem_close(numUsed) == -1 || sem_close(numFree) == -1)
    {
        fprintf(stderr, "Failure in semaphore close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(SEM_WRITE_MUTEX) == -1 || sem_unlink(SEM_NUM_USED) == -1 || sem_unlink(SEM_NUM_FREE) == -1)
    {
        fprintf(stderr, "Failure in semaphore unlink");
        exit(EXIT_FAILURE);
    }

    // unmap shared memory:
    if (munmap(myshm, sizeof(*myshm)) == -1)
    {
        fprintf(stderr, "error in munmap\n");
        exit(EXIT_FAILURE);
    }

    // remove shared memory object:
    if (shm_unlink(SHM_NAME) == -1)
    {
        fprintf(stderr, "error in shared memory unlink\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
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
