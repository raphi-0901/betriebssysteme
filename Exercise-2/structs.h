
#ifndef BETRIEBSSYSTEME_STRUCTS_H
#define BETRIEBSSYSTEME_STRUCTS_H

enum Color
{
    UNASSIGNED = -1,
    RED = 0,
    GREEN = 1,
    BLUE = 2,
};

struct Vertex
{
    int id;
    enum Color color;
};

struct Edge
{
    struct Vertex vertex1;
    struct Vertex vertex2;
};


#endif // BETRIEBSSYSTEME_STRUCTS_H
