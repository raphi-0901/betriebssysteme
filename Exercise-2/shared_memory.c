#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHM_NAME "/myshm"
#define MAX_DATA (50)

struct myshm {
    unsigned int state;
    unsigned int data[8];
};

static void openOrCreateSharedMemory(struct myshm *myshm) {
    // create and/or open the shared memory object:
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1) {
        fprintf(stderr, "error while creating shared memory");
    }

    // set the size of the shared memory:
    if (ftruncate(shmfd, sizeof(struct myshm)) < 0) {
        fprintf(stderr, "error in ftruncate");
    }

    // map shared memory object:
    myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE,
                 MAP_SHARED, shmfd, 0);
    if (myshm == MAP_FAILED) {
        fprintf(stderr, "error in mmap");
    }

    // close file descriptor
    if (close(shmfd) == -1) {
        fprintf(stderr, "error while closing shared memory");
    }
}

static void closeSharedMemory(struct myshm *myshm) {
    // unmap shared memory:
    if (munmap(myshm, sizeof(*myshm)) == -1) {
        fprintf(stderr, "error in munmap");
    }

    // remove shared memory object:
    if (shm_unlink(SHM_NAME) == -1) {
        fprintf(stderr, "error in shared memory unlink");
    }
}