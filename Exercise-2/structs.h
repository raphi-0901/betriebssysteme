
#ifndef BETRIEBSSYSTEME_STRUCTS_H
#define BETRIEBSSYSTEME_STRUCTS_H

#define SHM_NAME "/12220836shm"
#define MAX_DATA (500)

#include <stdbool.h>
#include <semaphore.h>

enum Color
{
    UNASSIGNED = -1,
    RED = 0,
    GREEN = 1,
    BLUE = 2,
};

typedef struct Vertex
{
    unsigned int id;
    enum Color color;
} Vertex;

typedef struct Edge
{
    Vertex vertex1;
    Vertex vertex2;
} Edge;

typedef struct EdgeDTO
{
    Edge results[128];
    unsigned int edgeCount;
} EdgeDTO;

typedef struct Shm_t
{
    bool terminateGenerators;
    // unsigned int numGenerators;

    EdgeDTO buffer[MAX_DATA];
    unsigned int readIndex;
    unsigned int writeIndex;

    sem_t writeMutex; // write mutex for generators
    sem_t numUsed;    // num of used indices in buffer (= supervisor should read)
    sem_t numFree;    // num of used indices in buffer (= generator can write)
} Shm_t;

#endif // BETRIEBSSYSTEME_STRUCTS_H
