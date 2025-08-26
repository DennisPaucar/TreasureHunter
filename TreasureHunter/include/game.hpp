#ifndef GAME_HPP
#define GAME_HPP

#include <raylib.h>
#include <raymath.h>
#include <cstdio>
#include <limits>
#include "types.hpp"
#include "graph.hpp"
#include "player.hpp"
#include "ui.hpp"
#include "netplay.hpp"
#include "rng.hpp"
#include <string>

using namespace std;
struct Game {
    bool online=false;
    bool isHost=false;
    bool onlineReady = false;
    bool setupOnce = false;


    NetPlay net;
    unsigned syncSeed=12345u;// semilla aleatoria
    Graph g; Vector2 pos[MAXN]; int treasure; Player p1,p2; int currentTurn,turnCount; GameCfg cfg; LevelCfg level; int W,H; CameraUI ui; bool mustPass; int fightMsgFrames; char fightMsg[64];
    // --- Estados del juego --> Protocolo VM ---
    enum State { STATE_MENU, STATE_PLAYING, STATE_PAUSED };
    State state;

    //--- Conexion para Partida---
    enum OnlineMode { OM_LOCAL=0, OM_HOST=1, OM_JOIN=2 };
    OnlineMode onlineMode = OM_LOCAL;

    char ipBuf[64]   = "159.203.111.148"; //Ip de la maquina virtual
    char portBuf[8]  = "5000"; //Puerto
    bool ipFocus     = false;
    bool portFocus   = false;
    bool connectFail = false;

    int   levelIndex = 1; // Inciar en nivel 1

    //--- Proceso de serializacion para enviar protocolos de estado ---
    string stateLine() const;
    void applyStateLine(const string& line);

    // --- Menu---
    bool  btnHoverStart1, btnHoverStart2, btnHoverStart3, btnHoverExit;
    void  updateMenu();
    void  drawMenu();

    //Probar Conexion y validar la conexion de los jugadores
    bool enableOnline(bool host, const char* ip, uint16_t port);


    // Duelo visual
    bool inFight; int fightNode; int fightFrames; int fightFramesMax; int fightWinner; int fightMoverIdx; float fightGauge;

    //Texxturas del juego
    Texture2D treasureTex; Texture2D roomTex; float roomSize;
    Texture2D corridorTex; float corridorWidth;
    Texture2D corridorTexBlocked;
    Texture2D menuBgTex;
    Texture2D p1Tex{};
    Texture2D p2Tex{};

    //Tamaño de los sprites de los jugadores
    float playerSpriteSize = 80.0f;
    float playerSpriteW = 80.0f;
    float playerSpriteH = 100.0f;

    bool scoresInit;

    // --- Ciclo de vida principal del juego ---
    void run();

    // ---Dibujo y carga de elementos del juego ---
    void layoutCircle(int n,float R,Vector2 c);
    bool pathExistsFrom(int src) const;
    bool pathsExistForBothPlayers() const;
    void shuffleTrapsSafe(float p);
    void setupLevel(int idx);
    void drawHintPath();
    void drawNeighborHighlights();
    bool edgeOpen(int u, int v) const;
    // -- Funciones de batlla --
    void applyMove(Player &p, int target);
    void startFight(int moverIdx, int target);
    void updateFight();
    void drawFightUI();
    // --Movimiento de los jugadores--
    bool tryMove(Player &p, int target);
    bool someoneWon() const;
    const char* winnerText() const;
    void nextTurn();
    int  pickNode(Vector2 screenMouse) const;

    //Aplciar las aristas bloqueadas con trampas a ambos jugadores
    void applyBlockedList(const vector<std::pair<int,int>>& bl);
    //Registro de Callbacks
    void bindNetCallbacks();
    //Lista de aristas bloqueadas
    vector<pair<int,int>> getBlockedList() const;
    Game() {
        net.onSetup = [this](int lvl, unsigned seed){
            printf("[JOIN] Recibido SETUP: lvl=%d, seed=%u\n", lvl, seed);
            levelIndex = lvl;
            syncSeed = seed;
            RNGSeed(syncSeed);
            setupLevel(levelIndex);
        };

        net.onStateLine = [this](const std::string& s){
            printf("[JOIN] Recibido STATE: %s\n", s.c_str());
            applyStateLine(s);
        };
    }

    // === Animacion ===
    struct SpriteAnim {
        int cols = 1, rows = 1;
        int frame = 0;
        int row = 0;
        float fps = 8.0f;
        float acc = 0.0f;
        bool playing = false;
    };
    // Cambiar movimiento de sprite
    SpriteAnim p1Anim{}, p2Anim{};
    const float MOVE_DUR = 0.25f;
    float p1MoveT = 1.0f;
    float p2MoveT = 1.0f;

};



#endif // GAME_HPP

