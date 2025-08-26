#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <limits>
#include <raylib.h>
#include <raymath.h>
#include "config.hpp"
#include "types.hpp"

struct Graph {
    int n;
    Edge adj[MAXN][MAXDEG];
    int  deg[MAXN];

    void init(int n_);
    bool existsEdge(int u,int v) const;
    void addUndirected(int u,int v,float w);
    int  findEdgeIdx(int u,int v);

    void shuffleTraps(float p);

    // Dijkstra. Si ignoreBlocked=true, ignora estado de trampas
    void dijkstra(int src, float dist[MAXN], int parent[MAXN], bool ignoreBlocked=false) const;
    // Atajo: ignora trampas
    void dijkstraAllEdges(int src, float dist[MAXN], int parent[MAXN]) const;

    int neighborsOpen(int u,int out[MAXDEG]) const;
};

#endif // GRAPH_HPP
