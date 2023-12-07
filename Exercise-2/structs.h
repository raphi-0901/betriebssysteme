
/**
 * @file structs.h
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Defines some aliases and structs used within all files.
 *
 **/

#ifndef BETRIEBSSYSTEME_STRUCTS_H
#define BETRIEBSSYSTEME_STRUCTS_H

#define SHM_NAME "/12220836shm"
#define MAX_DATA (500)
#define SEM_WRITE_MUTEX "/write_mutex"
#define SEM_NUM_USED "/num_used"
#define SEM_NUM_FREE "/num_free"

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
    unsigned int countGenerator;

    EdgeDTO buffer[MAX_DATA];
    unsigned int readIndex;
    unsigned int writeIndex;
} Shm_t;

#endif // BETRIEBSSYSTEME_STRUCTS_H
