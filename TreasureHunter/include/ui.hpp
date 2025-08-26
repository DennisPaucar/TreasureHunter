#ifndef UI_HPP
#define UI_HPP

#include <raylib.h>
#include <raymath.h>
#include "types.hpp"
#include "graph.hpp"

// Funciones de dibujo
struct CameraUI {
    Camera2D cam;
    int W{1280}, H{720};

    void init(int w,int h);
    void controls(float dt);
    void onResize(int w,int h);
};

// Renders para sprites
namespace DrawKit {
    void Graph(const ::Graph& g, const Vector2 pos[MAXN], int treasure);
    void Players(const Player& p1, const Player& p2, const Vector2 pos[MAXN]);
    void Token(const Player& p, const Vector2 pos[MAXN]);

    // NUEVO: Texturas en cada vértice
    void Rooms(const ::Graph& g, const Vector2 pos[MAXN], int treasure,
               Texture2D roomTex, Texture2D treasureTex, float roomSize);

    void Corridors(const ::Graph& g, const Vector2 pos[MAXN],
                   Texture2D corridorTexOpen,
                   Texture2D corridorTexBlocked,
                   float corridorWidth);
}

#endif // UI_HPP
