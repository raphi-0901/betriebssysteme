//
// Created by rwirnsberger on 12.11.23.
//

#ifndef BETRIEBSSYSTEME_SHARED_MEMORY_H
#define BETRIEBSSYSTEME_SHARED_MEMORY_H

#define SHM_NAME "/12220836shm"

#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include "structs.h"

static int openOrCreateSharedMemory(Shm_t **myshm)
{
    // create and/or open the shared memory object:
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
    {
        // Der Shared Memory existiert bereits, öffnen Sie ihn nur
        shmfd = shm_open(SHM_NAME, O_RDWR, 0);
        if (shmfd == -1)
        {
            fprintf(stderr, "error while opening shared memory\n");
            return -1;
        }
    }

    // set the size of the shared memory:
    if (ftruncate(shmfd, sizeof(*myshm)) < 0)
    {
        fprintf(stderr, "error in ftruncate\n");
        return -1;
    }

    // map shared memory object:
    *myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE,
                  MAP_SHARED, shmfd, 0);
    if (myshm == MAP_FAILED)
    {
        fprintf(stderr, "error in mmap\n");
        return -1;
    }

    // close file descriptor
    if (close(shmfd) == -1)
    {
        fprintf(stderr, "error while closing shared memory\n");
        return -1;
    }

    return 0;
}

static void closeSharedMemory(Shm_t *myshm, bool unlinkShm)
{
    // unmap shared memory:
    if (munmap(myshm, sizeof(*myshm)) == -1)
    {
        fprintf(stderr, "error in munmap\n");
    }

    // remove shared memory object:
    if (unlinkShm && shm_unlink(SHM_NAME) == -1)
    {
        fprintf(stderr, "error in shared memory unlink\n");
    }
}

#endif // BETRIEBSSYSTEME_SHARED_MEMORY_H
