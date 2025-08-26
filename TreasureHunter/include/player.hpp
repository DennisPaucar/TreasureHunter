#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "types.hpp"

inline Player makePlayer(int start, Color c){
    Player p; p.node=start; p.lastNode=start; p.score=50; p.color=c;
    for(int i=0;i<MAXN;++i) p.visited[i]=false; p.visited[start]=true; return p;
}

#endif // PLAYER_HPP
