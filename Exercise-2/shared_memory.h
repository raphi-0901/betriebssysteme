//
// Created by rwirnsberger on 12.11.23.
//

#ifndef BETRIEBSSYSTEME_SHARED_MEMORY_H
#define BETRIEBSSYSTEME_SHARED_MEMORY_H

struct myshm {
    unsigned int state;
    unsigned int data[8];
};

static void openOrCreateSharedMemory(struct myshm *myshm);
static void closeSharedMemory(struct myshm *myshm);
#endif //BETRIEBSSYSTEME_SHARED_MEMORY_H
