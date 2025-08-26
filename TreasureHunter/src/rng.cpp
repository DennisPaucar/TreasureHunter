#include "../include/rng.hpp"
void RNGSeed(unsigned seed){ std::srand(seed); }
int  RNGInt(int a, int b){ return a + (std::rand() % (b - a + 1)); }

