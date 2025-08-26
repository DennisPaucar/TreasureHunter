#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP
#include "types.hpp"

// dist y prev son arreglos de tamaño MAX_NODOS
void Dijkstra(const AdjList* g, int src, int dist[MAX_NODOS], int prev[MAX_NODOS],
              const bool blocked[MAX_NODOS][MAX_NODOS]);

#endif

