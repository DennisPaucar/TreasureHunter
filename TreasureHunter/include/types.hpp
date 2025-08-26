#ifndef TYPES_HPP
#define TYPES_HPP

#include <raylib.h>
#include "config.hpp"

struct Edge { int to; float w; bool blocked; };

struct LevelCfg { int nodes; int extraEdges; float trapProb; bool trapsEveryTurn; };
struct GameCfg  { int maxTurns; int fontSize; };

struct Player {
    int node;
    int lastNode;
    int score;
    bool visited[MAXN];
    Color color;
};

#endif // TYPES_HPP
