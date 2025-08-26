#ifndef HEAP_HPP
#define HEAP_HPP
#include "types.hpp"

void HeapInit(MinHeap* h, int n);
bool HeapEmpty(const MinHeap* h);
void HeapPush(MinHeap* h, int node, int dist);
int  HeapPop(MinHeap* h);
void HeapDecreaseKey(MinHeap* h, int node, int newDist);

#endif

