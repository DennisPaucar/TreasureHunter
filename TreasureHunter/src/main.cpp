#include "../include/game.hpp"
#include <cstring>
#include <cstdio>

int main(int argc, char** argv){
    Game game;

    // Modo por argumentos:
    //   --host  => autoridad (Jugador 1)
    //   --join  => cliente (Jugador 2)
    if (argc >= 2){
        if (std::strcmp(argv[1], "--host") == 0){
            game.enableOnline(true,  "159.203.111.148", 5000);
            std::puts("[MAIN] Modo ONLINE HOST");
        } else if (std::strcmp(argv[1], "--join") == 0){
            game.enableOnline(false, "159.203.111.148", 5000);
            std::puts("[MAIN] Modo ONLINE JOIN");
        } else {
            std::puts("[MAIN] Modo local (sin red)");
        }
    } else {
        std::puts("[MAIN] Modo local (sin red)");
    }

    game.run();
    return 0;
}
