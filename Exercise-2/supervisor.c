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

#define SHM_NAME "/myshm"
#define MAX_DATA (50)

struct myshm
{
    unsigned int state;
    unsigned int data[MAX_DATA];
};

static void usage(char *name);
static void openOrCreateSharedMemory(struct myshm *myshm);
static void closeSharedMemory(struct myshm *myshm);

int delay = 0;
int limit = -1;
char *name;
int main(int argc, char **argv)
{
    struct myshm myshm;
    openOrCreateSharedMemory(&myshm);

    int opt;
    while ((opt = getopt(argc, argv, "n:w:")) != -1)
    {
        switch (opt)
        {
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

    size_t len = 0;
    char *line = NULL;
    while (getline(&line, &len, stdin) != -1)
    {
        printf("line: %s", line);
    }

    closeSharedMemory(&myshm);
}

static void openOrCreateSharedMemory(struct myshm *myshm)
{
    // create and/or open the shared memory object:
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
    {
        fprintf(stderr, "error while creating shared memory");
    }

    // set the size of the shared memory:
    if (ftruncate(shmfd, sizeof(struct myshm)) < 0)
    {
        fprintf(stderr, "error in ftruncate");
    }

    // map shared memory object:
    myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE,
                 MAP_SHARED, shmfd, 0);
    if (myshm == MAP_FAILED)
    {
        fprintf(stderr, "error in mmap");
    }

    // close file descriptor
    if (close(shmfd) == -1)
    {
        fprintf(stderr, "error while closing shared memory");
    }
}

static void closeSharedMemory(struct myshm *myshm)
{
    // unmap shared memory:
    if (munmap(myshm, sizeof(*myshm)) == -1)
    {
        fprintf(stderr, "error in munmap");
    }

    // remove shared memory object:
    if (shm_unlink(SHM_NAME) == -1)
    {
        fprintf(stderr, "error in shared memory unlink");
    }
}

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage(char *name)
{
    fprintf(stderr, "Usage:\t%s [-n limit] [-w delay]\n", name);
    exit(EXIT_FAILURE);
}