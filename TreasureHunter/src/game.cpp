#include "../include/game.hpp"
#include "../include/rng.hpp"
#include <cstring>

using namespace std;

//---Guia de ayuda nav bar---
static bool TextBoxSimple(Rectangle r, char* buf, int maxLen, bool digitsOnly, bool &focus){

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        focus = (m.x >= r.x && m.x <= r.x+r.width && m.y >= r.y && m.y <= r.y+r.height);
    }

    if (focus){
        int c;
        while ((c = GetCharPressed()) != 0){
            if (digitsOnly && (c < '0' || c > '9')) continue;
            int len = (int)strlen(buf);
            if (len < maxLen-1 && c >= 32 && c < 127){
                buf[len] = (char)c; buf[len+1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE)){
            int len = (int)strlen(buf);
            if (len > 0) buf[len-1] = '\0';
        }
    }


    DrawRectangleRec(r, (Color){36,36,42,220});
    DrawRectangleLinesEx(r, 2, focus ? (Color){50,130,220,220} : (Color){120,120,120,180});
    DrawText(buf, (int)r.x+8, (int)r.y+10, 20, RAYWHITE);
    return focus;
}
// -- Validacion para conexion a online --
bool Game::enableOnline(bool host, const char* ip, uint16_t port){
    bool ok = net.connect(ip, port, host);
    online = ok;
    isHost = (ok && host);
    connectFail = !ok;
    if (ok){
        bindNetCallbacks();
        onlineReady = false;
        setupOnce   = false;
    } else {
        onlineMode = OM_LOCAL;
    }
    return ok;
}


// -- Funcion para validar recibimiento de paquetes para el J2 --
std::string Game::stateLine() const {

    return TextFormat("turn=%d p1node=%d p1score=%d p2node=%d p2score=%d treasure=%d turnCount=%d mustPass=%d",
        currentTurn, p1.node, p1.score, p2.node, p2.score, treasure, turnCount, (int)mustPass);
}

// -- Funcion para validar Envio de paquetes del J1--
void Game::applyStateLine(const std::string& line){
    int turn, pn1, sc1, pn2, sc2, tre, tc, mp;
    if (sscanf(line.c_str(),
        "turn=%d p1node=%d p1score=%d p2node=%d p2score=%d treasure=%d turnCount=%d mustPass=%d",
        &turn,&pn1,&sc1,&pn2,&sc2,&tre,&tc,&mp)==8)
    {
        currentTurn=turn; p1.node=pn1; p1.score=sc1; p2.node=pn2; p2.score=sc2;
        treasure=tre; turnCount=tc; mustPass=(mp!=0);
    }
}

// Dibujo Del vertice del grafo
void Game::layoutCircle(int n,float R,Vector2 c){ for(int i=0;i<n;++i){ float ang=(2.0f*PI*i)/n - PI/2.0f; pos[i] = { c.x + R*cosf(ang), c.y + R*sinf(ang) }; } }
// -Dibujo de linea Guia por Dijkstra --
bool Game::pathExistsFrom(int src) const { float dist[MAXN]; int parent[MAXN]; g.dijkstra(src, dist, parent); return dist[treasure] != std::numeric_limits<float>::infinity(); }

bool Game::pathsExistForBothPlayers() const { return pathExistsFrom(p1.node) && pathExistsFrom(p2.node); }
// Validador de trampas
void Game::shuffleTrapsSafe(float p){
    int th=(int)(p*1000.0f);
    for(int u=0; u<g.n; ++u){
        for(int i=0; i<g.deg[u]; ++i){
            int v = g.adj[u][i].to; if(u>=v) continue;
            bool newState = (GetRandomValue(0,999) < th);
            if(g.adj[u][i].blocked == newState) continue;
            bool old = g.adj[u][i].blocked;
            g.adj[u][i].blocked = newState; int j=g.findEdgeIdx(v,u); if(j!=-1) g.adj[v][j].blocked = newState;
            if(!pathsExistForBothPlayers()){
                g.adj[u][i].blocked = old; if(j!=-1) g.adj[v][j].blocked = old;
            }
        }
    }
}
//Dibujado del mapa
void Game::setupLevel(int idx){
    if(idx==1) level={10,2,0.15f,false};
    else if(idx==2) level={15,4,0.30f,false};
    else            level={20,6,0.35f,true};

    g.init(level.nodes);
    ui.init(GetScreenWidth(), GetScreenHeight());
    W=ui.W; H=ui.H;

    // mundo centrado en (0,0)
    layoutCircle(g.n, (float)W*0.45f, (Vector2){0,0});


    for(int i=0;i<g.n;++i){ int j=(i+1)%g.n; g.addUndirected(i,j,Vector2Distance(pos[i],pos[j])); }
    // aristas extra
    for(int k=0;k<level.extraEdges;++k){ int a=GetRandomValue(0,g.n-1), b=GetRandomValue(0,g.n-1); if(a==b){--k; continue;} g.addUndirected(a,b,Vector2Distance(pos[a],pos[b])); }

    // Jugadores aleatorios
    int a=GetRandomValue(0,g.n-1); int b; do{ b=GetRandomValue(0,g.n-1);}while(b==a);
    p1=makePlayer(a,(Color){80,180,255,255});
    p2=makePlayer(b,(Color){255,120,90,255});

    // Tesoro lejos de ambos (por Dijkstra )
    const float INF = std::numeric_limits<float>::infinity();
    float d1[MAXN], d2[MAXN]; int pa[MAXN];
    g.dijkstraAllEdges(p1.node, d1, pa);
    g.dijkstraAllEdges(p2.node, d2, pa);
    int bestT = -1; float bestScore = -1.0f; float bestTie=-1.0f;
    for(int t=0;t<g.n;++t){
        if(t==p1.node || t==p2.node) continue;
        if(d1[t]==INF || d2[t]==INF) continue;
        float score = (d1[t] < d2[t] ? d1[t] : d2[t]);
        float tie   = d1[t] + d2[t];
        if(score > bestScore || (score==bestScore && tie>bestTie)){ bestScore=score; bestTie=tie; bestT=t; }
    }
    if(bestT==-1){ do{ bestT=GetRandomValue(0,g.n-1);}while(bestT==p1.node || bestT==p2.node); }
    treasure = bestT;

    // Trampas seguras al iniciar
    shuffleTrapsSafe(level.trapProb);
    turnCount=0; currentTurn=1; mustPass=false;

    fightMsgFrames = 0; fightMsg[0] = '\0';
    inFight = false; fightNode = -1; fightFrames = 0; fightFramesMax = 60; fightWinner = 0; fightMoverIdx = 0; fightGauge = 0.5f;
}
//Dibujo de la linea guia hacia el tesoro
void Game::drawHintPath(){
    int src=currentTurn==1?p1.node:p2.node; float dist[MAXN]; int parent[MAXN]; g.dijkstra(src,dist,parent);
    if(dist[treasure]==std::numeric_limits<float>::infinity()) return;
    int path[MAXN]; int k=0; int cur=treasure; while(cur!=-1&&k<MAXN){ path[k++]=cur; if(cur==src) break; cur=parent[cur]; }
    for(int i=k-1;i>0;--i){ int u=path[i], v=path[i-1]; DrawLineEx(pos[u],pos[v],5.0f,(Color){80,220,120,180}); }
}

void Game::drawNeighborHighlights(){ int u=currentTurn==1?p1.node:p2.node; if(mustPass) return; int nb[MAXDEG]; int k=g.neighborsOpen(u,nb); for(int i=0;i<k;++i) DrawCircleLines((int)pos[nb[i]].x,(int)pos[nb[i]].y,22,(Color){120,220,255,200}); }

bool Game::edgeOpen(int u, int v) const { for(int i=0;i<g.deg[u];++i){ Edge e = g.adj[u][i]; if(e.to==v && !e.blocked) return true; } return false; }

//Movimiento del Jugador entre vertices
void Game::applyMove(Player &p, int target){
    p.lastNode = p.node;
    p.node = target;

    if (&p == &p1) { p1MoveT = 0.0f; p1Anim.playing = true; }
    else           { p2MoveT = 0.0f; p2Anim.playing = true; }

    if(!p.visited[p.node]){ p.visited[p.node]=true; p.score+=50; }
    if(p.node==treasure) p.score+=300;
}

//Lucha por el vertice
void Game::startFight(int moverIdx, int target){
    inFight = true; fightNode = target; fightFrames = 0; fightWinner = 0; fightMoverIdx = moverIdx; fightGauge = 0.5f;
    const char* who = (moverIdx==1) ? "J1" : "J2";
    snprintf(fightMsg, sizeof(fightMsg), "¡Duelo por el nodo %d! %s vs rival", target, who);
    fightMsgFrames = 90;
}

//Resultado de la lucha por el vertice
void Game::updateFight(){
    float jitter = (float)GetRandomValue(0,100) / 100.0f;
    float t = (float)fightFrames / (float)fightFramesMax;
    float ease = 1.0f - t; if(ease<0) ease=0;
    fightGauge = fightGauge*0.6f + jitter*0.4f*ease + 0.2f*(0.5f - fightGauge)*(1.0f-ease);

    fightFrames++;
    if(fightFrames >= fightFramesMax){
        if(fightWinner==0){
            bool moverWins = (GetRandomValue(0,1)==1);
            fightWinner = moverWins ? 1 : 2;

            Player &mover = (fightMoverIdx==1? p1 : p2);
            Player &def   = (fightMoverIdx==1? p2 : p1);
            (void)def; // no se usa por ahora

            if(moverWins){
                applyMove(mover, fightNode);
                snprintf(fightMsg, sizeof(fightMsg), "Choque en nodo %d: %s avanza", fightNode, (fightMoverIdx==1?"J1":"J2"));
            } else {
                snprintf(fightMsg, sizeof(fightMsg), "Choque en nodo %d: %s pierde prioridad", fightNode, (fightMoverIdx==1?"J1":"J2"));
            }
            fightMsgFrames = 90; inFight = false; nextTurn();
            if (online && isHost) {
                net.sendFightRes(fightWinner);
                net.sendBlockedList(getBlockedList());
                net.sendStateLine(stateLine());
            }
        }
    }
}
//Dibujado de la lucha por el vertice
void Game::drawFightUI(){
    int panelW = 420, panelH = 120; int px = W/2 - panelW/2, py = 100;
    DrawRectangle(px, py, panelW, panelH, (Color){16,16,16,240});
    DrawRectangleLines(px, py, panelW, panelH, (Color){220,220,220,180});
    DrawText("DUEL0", px+16, py+10, 20, RAYWHITE);
    DrawText(TextFormat("Nodo %d", fightNode), px+300, py+10, 20, YELLOW);
    int bx = px+30, by = py+58, bw = panelW-60, bh = 18;
    DrawRectangle(bx, by, bw, bh, (Color){40,40,40,255});
    DrawRectangleLines(bx, by, bw, bh, (Color){200,200,200,130});
    DrawRectangle(bx, by, bw/2, bh, (Color){80,180,255,120});
    DrawRectangle(bx + bw/2, by, bw/2, bh, (Color){255,120,90,120});
    int mx = bx + (int)(fightGauge * (float)(bw-10));
    DrawRectangle(mx, by-6, 10, bh+12, (Color){250,250,250,230});
    DrawText("J1", bx+6,  by+bh+6, 18, (Color){80,180,255,255});
    DrawText("J2", bx+bw-30, by+bh+6, 18, (Color){255,120,90,255});
    DrawText(TextFormat("Resolviendo..."), px+16, py+34, 18, LIGHTGRAY);
}

bool Game::tryMove(Player &p, int target){
    if(target<0 || target>=g.n) return false;

    bool ok=false; for(int i=0;i<g.deg[p.node];++i){ Edge e=g.adj[p.node][i]; if(e.to==target && !e.blocked){ ok=true; break; } }
    if(!ok) return false;

    Player &other = (&p==&p1)?p2:p1;
    if(other.node == target){
        bool moverWins = (GetRandomValue(0,1)==1); const char* who = (&p==&p1) ? "J1" : "J2";
        if(!moverWins){ snprintf(fightMsg, sizeof(fightMsg), "Choque en nodo %d: %s pierde prioridad", target, who); fightMsgFrames = 90; return true; }
        else {snprintf(fightMsg, sizeof(fightMsg), "Choque en nodo %d: %s avanza", target, who); fightMsgFrames = 90; }
    }

    p.lastNode = p.node; p.node = target; if(!p.visited[p.node]){ p.visited[p.node]=true; p.score+=50; } if(p.node==treasure) p.score+=300; return true;
}

bool Game::someoneWon() const { return p1.node==treasure || p2.node==treasure || turnCount>=cfg.maxTurns; }


//Considerar el uso de vectores para el protocolo de la Vm, los procesos generales del juego mantiene el uso de estructuras
void Game::applyBlockedList(const std::vector<std::pair<int,int>>& bl){

    for (int u=0; u<g.n; ++u)
        for (int i=0; i<g.deg[u]; ++i)
            g.adj[u][i].blocked = false;

    for (auto [u,v] : bl){
        int i = g.findEdgeIdx(u,v); if (i!=-1) g.adj[u][i].blocked = true;
        int j = g.findEdgeIdx(v,u); if (j!=-1) g.adj[v][j].blocked = true;
    }
}


std::vector<std::pair<int,int>> Game::getBlockedList() const {
    std::vector<std::pair<int,int>> bl;
    for (int u=0; u<g.n; ++u){
        for (int i=0; i<g.deg[u]; ++i){
            const Edge &e = g.adj[u][i];
            if (u < e.to && e.blocked) bl.emplace_back(u, e.to);
        }
    }
    return bl;
}

const char* Game::winnerText() const {
    if(p1.node==treasure&&p2.node!=treasure) return " Ganador: J1 (tesoro)";
    if(p2.node==treasure&&p1.node!=treasure) return " Ganador: J2 (tesoro)";
    if(turnCount>=cfg.maxTurns){ if(p1.score>p2.score) return " Ganador por puntos: J1"; if(p2.score>p1.score) return " Ganador por puntos: J2"; return "Empate"; }
    return "";
}

void Game::nextTurn(){
    if (!online || isHost) {
        shuffleTrapsSafe(level.trapProb);
    }
    currentTurn = (currentTurn==1?2:1);
    if (currentTurn==1) ++turnCount;

    if (online && isHost) {
        net.sendBlockedList(getBlockedList());
        net.sendStateLine(stateLine());
    }
}

int Game::pickNode(Vector2 screenMouse) const { Vector2 m = GetScreenToWorld2D(screenMouse, ui.cam); for(int i=0;i<g.n;++i) if(Vector2Distance(m,pos[i])<=18.0f) return i; return -1; }

static bool PointInRect(Vector2 p, Rectangle r){
    return (p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height);
}
//Diseño de los botones del menu
static void DrawButton(Rectangle r, const char* text, bool hovered){
    Color bg = hovered ? (Color){50, 130, 220, 220} : (Color){36, 36, 42, 220};
    DrawRectangleRounded(r, 0.2f, 8, bg);
    DrawRectangleRoundedLines(r, 0.2f, 8, (Color){220,220,220, (unsigned char)(hovered?220:120)});
    int tw = MeasureText(text, 22);
    DrawText(text, (int)(r.x + (r.width - tw)/2), (int)(r.y + (r.height-22)/2), 22, RAYWHITE);
}

//Dibujado del menu principal
void Game::updateMenu(){
    Vector2 m = GetMousePosition();
    // --- Panel Online (posiciones) ---
    float panelX = W*0.5f - 360;
    float panelY = H*0.22f + 80;

    Rectangle btnLocal = { panelX +180,         panelY+110,     110, 40 };
    Rectangle btnHost  = { panelX + 120.0f +180,panelY+110,     110, 40 };
    Rectangle btnJoin  = { panelX + 240.0f+180,panelY+110,     110, 40 };
    Rectangle connectR = { panelX + 360.0f+180,panelY+52.0f+110,140, 40 };

    // Layout centrado
    float bw = 300, bh = 48;
    float cx = W*0.5f - bw*0.5f;
    float cy = H*0.5f - 2*bh - 12;

    //Botones de niveles
    Rectangle r1 = { cx, cy +150, bw, bh };           // Nivel 1
    Rectangle r2 = { cx, cy + (bh+12)+150, bw, bh }; // Nivel 2
    Rectangle r3 = { cx, cy + 2*(bh+12)+150, bw, bh }; // Nivel 3
    Rectangle rE = { cx, cy + 3*(bh+12) + 8 +150, bw, bh }; // Salir

    btnHoverStart1 = PointInRect(m, r1);
    btnHoverStart2 = PointInRect(m, r2);
    btnHoverStart3 = PointInRect(m, r3);
    btnHoverExit   = PointInRect(m, rE);

    auto in = [&](Rectangle r){
        return m.x>=r.x && m.x<=r.x+r.width && m.y>=r.y && m.y<=r.y+r.height;
    };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // --- Selector de modo ---
        if (in(btnLocal)) {
            onlineMode = OM_LOCAL;
            online = false;
            isHost = false;
            connectFail = false;
        }
        else if (in(btnHost))  {
            onlineMode = OM_HOST;
            connectFail = false;
        }
        else if (in(btnJoin))  {
            onlineMode = OM_JOIN;
            connectFail = false;
        }
        // ---  Conectar / Desconectar ---
        else if (in(connectR)) {
            if (onlineMode == OM_LOCAL){
                // salir de online si estuviera activo
                if (online) net.disconnect();
                online = false;
                isHost = false;
                connectFail = false;
            } else {
                int p = atoi(portBuf);
                if (p <= 0 || p > 65535) {
                    connectFail = true;
                } else {
                    bool ok = enableOnline(onlineMode==OM_HOST, ipBuf, (uint16_t)p);
                    connectFail = !ok;
                }
            }
        }

        //---Empezar nivel---
        auto startGame = [this](int lvl){
            levelIndex = lvl;
            if (onlineMode != OM_LOCAL && !net.isConnected()){
                connectFail = true;
                return;
            }

            if (online) {
                if (isHost) {
                    // HOST: Proceso genera semilla
                    syncSeed = (unsigned)(GetTime()*1000) ^ 0xA5A5u;
                    RNGSeed(syncSeed);
                    SetRandomSeed(syncSeed);
                    setupLevel(levelIndex);
                    net.sendSetup(levelIndex, syncSeed);
                    net.sendBlockedList(getBlockedList());
                    net.sendStateLine(stateLine());
                    onlineReady = true;
                    setupOnce   = true;
                    state = STATE_PLAYING;
                } else {
                    // Proceso JOIN: NO se crea el nivel

                }
            } else {
                // Modo local (offline)
                setupLevel(levelIndex);
                state = STATE_PLAYING;
            }
        };

        if (btnHoverStart1)      startGame(1);
        else if (btnHoverStart2) startGame(2);
        else if (btnHoverStart3) startGame(3);
        else if (btnHoverExit)   CloseWindow();
    }

}

//Dibujo del menu
void Game::drawMenu(){

    ClearBackground((Color){20,20,26,255});
    DrawRectangleGradientV(0, 0, W, H, (Color){10,10,12,0}, (Color){10,10,12,160});

    if (menuBgTex.id != 0) {
        DrawTexturePro(
            menuBgTex,
            { 0, 0, (float)menuBgTex.width, (float)menuBgTex.height },
            { 0, 0, (float)W, (float)H },
            { 0, 0 },
            0.0f,
            WHITE
        );
    }

    DrawText("Treasure Hunter", W/2 - MeasureText("Treasure Hunter", 50)/2, 100+100, 50, GOLD);

    const char* title = "Cazador de Tesoro";
    int tw = MeasureText(title, 42);
    DrawText(title, W/2 - tw/2, (int)(H*0.22f)+100, 42, RAYWHITE);

    const char* subt = "Usa la camara con WASD/rueda, click para moverte";
    int sw = MeasureText(subt, 18);
    DrawText(subt, W/2 - sw/2, (int)(H*0.22f) + 46+100, 18, (Color){180,180,180,255});

    // --- Panel Online ---
    float panelX = W*0.5f - 360;
    float panelY = H*0.22f + 80;

    Rectangle btnLocal = { panelX + 180,          panelY +110,      110, 40 };
    Rectangle btnHost  = { panelX + 120.0f +180, panelY+110,      110, 40 };
    Rectangle btnJoin  = { panelX + 240.0f +180, panelY+110,      110, 40 };
    Rectangle connectR = { panelX + 360.0f +180, panelY+52.0f +110,140, 40 };

    auto drawModeBtn = [&](Rectangle r, const char* txt, bool on){
        DrawRectangleRounded(r, 0.2f, 8, on ? (Color){50,130,220,220} : (Color){36,36,42,220});
        DrawRectangleRoundedLines(r, 0.2f, 8, on ? (Color){220,220,220,220} : (Color){120,120,120,160});
        int t = MeasureText(txt, 20);
        DrawText(txt, (int)(r.x + (r.width - t)/2), (int)(r.y + 10), 20, RAYWHITE);
    };

    DrawText("Modo:", (int)panelX+180, (int)panelY - 20 +110, 18, GRAY);
    drawModeBtn(btnLocal, "Local", onlineMode==OM_LOCAL);
    drawModeBtn(btnHost , "J1",  onlineMode==OM_HOST);
    drawModeBtn(btnJoin , "J2",  onlineMode==OM_JOIN);


    DrawText(TextFormat("IP: %s", ipBuf),       (int)panelX+180,        (int)panelY+58+110, 18, LIGHTGRAY);
    DrawText(TextFormat("PUERTO: %s", portBuf), (int)panelX+140+180,    (int)panelY+58+110, 18, LIGHTGRAY);

    // Botón Conectar
    DrawRectangleRounded(connectR, 0.2f, 8, (Color){80,180,120,220});
    DrawRectangleRoundedLines(connectR, 0.2f, 8, (Color){220,220,220,200});
    int twc = MeasureText("Conectar", 20);
    DrawText("Conectar", (int)(connectR.x + (connectR.width - twc)/2), (int)(connectR.y + 10), 20, RAYWHITE);

    // Estado conexión
    if (online){
        DrawText(isHost ? "Conectado como HOST" : "Conectado como JOIN",
                 (int)panelX+180, (int)panelY+100+110, 18, (Color){120,220,160,255});
    } else if (connectFail){
        DrawText("No se pudo conectar.", (int)panelX+180, (int)panelY+100+110, 18, (Color){255,120,90,255});
    }

    // Botones
    float bw = 300, bh = 48;
    float cx = W*0.5f - bw*0.5f;
    float cy = H*0.5f - 2*bh - 12;

    Rectangle r1 = { cx, cy + 150, bw, bh };
    Rectangle r2 = { cx, cy + (bh+12)+150, bw, bh };
    Rectangle r3 = { cx, cy + 2*(bh+12)+150, bw, bh };
    Rectangle rE = { cx, cy + 3*(bh+12) + 8+150, bw, bh };

    DrawButton(r1, "Jugar: Nivel 1", btnHoverStart1);
    DrawButton(r2, "Jugar: Nivel 2", btnHoverStart2);
    DrawButton(r3, "Jugar: Nivel 3", btnHoverStart3);
    DrawButton(rE, "Salir", btnHoverExit);

    //Pie de pagina
    const char* footer = "ESC: salir / Pausa dentro del juego";
    int fw = MeasureText(footer, 16);
    DrawText(footer, W/2 - fw/2, H - 36, 16, (Color){160,160,160,255});
}

void Game::bindNetCallbacks(){
    net.onSetup = [this](int lvl, unsigned seed){
        printf("[JOIN] SETUP recibido (lvl=%d seed=%u)\n", lvl, seed);
        levelIndex = lvl; syncSeed = seed;
        RNGSeed(syncSeed); SetRandomSeed(syncSeed);
        setupLevel(levelIndex);
        onlineReady = true;
        state = STATE_PLAYING;
    };

    net.onMove = [this](int who, int to){
        Player& p = (who==1? p1 : p2);
        if (edgeOpen(p.node,to)){
            Player& other = (who==1? p2 : p1);
            if (other.node==to){
                if (isHost) startFight(who, to);
            } else {
                applyMove(p, to);
                nextTurn();
                if (isHost) net.sendStateLine(stateLine());
            }
        }
    };

    net.onPass = [this](int who){
        if (isHost){
            nextTurn();
            net.sendBlockedList(getBlockedList());
            net.sendStateLine(stateLine());
        }
    };

    net.onFightStart = [this](int mover, int node){ startFight(mover, node); };

    net.onFightRes = [this](int winner){
        fightWinner = winner; inFight = false;
        if (winner==1){
            Player &mover = (fightMoverIdx==1? p1 : p2);
            applyMove(mover, fightNode);
        }
        nextTurn();
        if (isHost) net.sendStateLine(stateLine());
    };

    net.onStateLine = [this](const string& s){ applyStateLine(s); };

    net.onBlockedList = [this](const vector<pair<int,int>>& bl){
        applyBlockedList(bl);
    };
}

static void drawPlayerSprite(Texture2D tex, Vector2 wp, float w, float h){
    if (tex.id == 0) return;
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = { wp.x, wp.y, w, h };
    Vector2 origin = { w*0.5f, h*0.5f };
    DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
}



void Game::run(){

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(1280,720,"Treasure Hunter");
    W=GetScreenWidth(); H=GetScreenHeight();
    cfg.maxTurns=60; cfg.fontSize=18;

    ui.init(W,H);

    //textura: Sala
    roomTex = LoadTexture("assets/room.png");
    if (roomTex.id != 0) SetTextureFilter(roomTex, TEXTURE_FILTER_BILINEAR);
    //textura: Tesoro
    treasureTex = LoadTexture("assets/treasure.png");
    if (treasureTex.id != 0) SetTextureFilter(treasureTex, TEXTURE_FILTER_BILINEAR);
    //textura: Corredores
    corridorTex = LoadTexture("assets/corridor.png");
    if (corridorTex.id != 0) SetTextureFilter(corridorTex, TEXTURE_FILTER_BILINEAR);
    //textura: Corredor con trampa
    corridorTexBlocked = LoadTexture("assets/corridor_blocked.jpg");
    if (corridorTexBlocked.id != 0) SetTextureFilter(corridorTexBlocked, TEXTURE_FILTER_BILINEAR);
    //textura: Fondo del menu
    menuBgTex = LoadTexture("assets/menu_bg.png");
    if (menuBgTex.id != 0) SetTextureFilter(menuBgTex, TEXTURE_FILTER_BILINEAR);
    //tectura: jugador 1
    p1Tex = LoadTexture("assets/p1.png");
    if (p1Tex.id != 0) SetTextureFilter(p1Tex, TEXTURE_FILTER_BILINEAR);
    //textura: jugador 2
    p2Tex = LoadTexture("assets/p2.png");
    if (p2Tex.id != 0) SetTextureFilter(p2Tex, TEXTURE_FILTER_BILINEAR);


    // Config del sprite de los jugadores
    p1Anim.cols = 4; p1Anim.rows = 4; p1Anim.fps = 10.0f;
    p2Anim.cols = 4; p2Anim.rows = 4; p2Anim.fps = 10.0f;

    //Tamaños de la sala y corredores
    corridorWidth = 35.0f;
    roomSize = 130.0f;


    state = STATE_MENU;
    //Frames por segundo
    SetTargetFPS(60);

    //Loop principal del juego
    while(!WindowShouldClose()){
        int newW = GetScreenWidth();
        int newH = GetScreenHeight();
        if (newW != W || newH != H) {
            W = newW; H = newH;
            ui.onResize(W, H);
        }

        float dt = GetFrameTime();
        if (online) net.pump();


        if (IsKeyPressed(KEY_ESCAPE)) {
            state = STATE_MENU;
            continue;
        }

        if (state == STATE_PLAYING){


            if (online && !isHost && !onlineReady) {
                printf("[JOIN] esperando SETUP del host...\n");
                BeginDrawing();
                ClearBackground(RAYWHITE);
                DrawText("Conectado. Esperando SETUP del Host...", 40, 40, 20, DARKGRAY);
                EndDrawing();
                continue;
            }
            ui.controls(dt);

            // Nivel 1
            if (IsKeyPressed(KEY_ONE)) {
                if (online) {
                    if (isHost) {
                        levelIndex = 1;
                        syncSeed = (unsigned)(GetTime()*1000) ^ 0xB4B4u;
                        RNGSeed(syncSeed);
                        SetRandomSeed(syncSeed);
                        setupLevel(levelIndex);
                        printf("[HOST] enviando SETUP lvl=%d seed=%u\n", levelIndex, syncSeed);
                        net.sendSetup(levelIndex, syncSeed);
                        net.sendBlockedList(getBlockedList());
                        net.sendStateLine(stateLine());
                        onlineReady = true;
                        setupOnce   = true;
                    } // si es JOIN, ignora
                } else {
                    levelIndex = 1; setupLevel(levelIndex);
                }
            }
            //Nivel 2
            if (IsKeyPressed(KEY_TWO)) {
                if (online) {
                    if (isHost) {
                        levelIndex = 2;
                        syncSeed = (unsigned)(GetTime()*1000) ^ 0xB4B4u;
                        RNGSeed(syncSeed);
                        SetRandomSeed(syncSeed);
                        setupLevel(levelIndex);
                        net.sendSetup(levelIndex, syncSeed);
                        net.sendBlockedList(getBlockedList());
                        net.sendStateLine(stateLine());
                        onlineReady = true;
                        setupOnce   = true;
                    }
                } else {
                    levelIndex = 2; setupLevel(levelIndex);
                }
            }
            //Nivel 3
            if (IsKeyPressed(KEY_THREE)) {
                if (online) {
                    if (isHost) {
                        levelIndex = 3;
                        syncSeed = (unsigned)(GetTime()*1000) ^ 0xB4B4u;
                        RNGSeed(syncSeed);
                        SetRandomSeed(syncSeed);
                        setupLevel(levelIndex);
                        net.sendSetup(levelIndex, syncSeed);
                        net.sendBlockedList(getBlockedList());
                        net.sendStateLine(stateLine());
                        onlineReady = true;
                        setupOnce   = true;
                    }
                } else {
                    levelIndex = 3; setupLevel(levelIndex);
                }
            }

            //Mostrar Batalla
            if(inFight){ updateFight(); }
            else {
                int uCheck = (currentTurn==1? p1.node : p2.node);
                int nblist[MAXDEG]; mustPass = (g.neighborsOpen(uCheck, nblist) == 0);

                if(!someoneWon()){
                    if (mustPass) {
                        if (IsKeyPressed(KEY_SPACE)) {
                            if (online) {
                                if (isHost) {
                                    nextTurn();
                                    net.sendBlockedList(getBlockedList());
                                    net.sendStateLine(stateLine());
                                } else {
                                    net.sendPass(currentTurn);
                                }
                            } else {
                                nextTurn();
                            }
                        }
                    }

                    else {
                        Player &curP  = (currentTurn==1 ? p1 : p2);
                        Player &other = (currentTurn==1 ? p2 : p1); (void)other;

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            int hit = pickNode(GetMousePosition());
                            if (hit != -1 && edgeOpen(curP.node, hit)) {
                                if (online) {
                                    bool myTurn = ( (isHost && currentTurn==1) || (!isHost && currentTurn==2) );
                                    if (!myTurn) {
                                        // Ingorar turno J2
                                    } else if (isHost) {
                                        // HOST: autoridad
                                        if ((currentTurn==1 ? p2.node : p1.node) == hit) {
                                            startFight((currentTurn==1?1:2), hit);
                                            // al resolver batlla  envíar protocolo --> FIGHTRES + STATE
                                        } else {
                                            applyMove(curP, hit);
                                            nextTurn();
                                            net.sendBlockedList(getBlockedList());
                                            net.sendStateLine(stateLine());
                                        }
                                    } else {
                                        // J2 ganador
                                        net.sendMove(currentTurn, hit);
                                    }
                                } else {
                                    // Modo local
                                    if ((currentTurn==1 ? p2.node : p1.node) == hit) {
                                        startFight((currentTurn==1?1:2), hit);
                                    } else {
                                        applyMove(curP, hit);
                                        nextTurn();
                                    }
                                }
                            }
                        }
                    }

                }
            }
        }else if (state == STATE_MENU){
            updateMenu();
        }


        BeginDrawing();

        if (state == STATE_MENU){
            drawMenu();
            EndDrawing();
            continue;
        }
        ClearBackground((Color){24,24,30,255});
        BeginMode2D(ui.cam);



        // dibujar salas en cada vértice
        DrawKit::Corridors(g, pos, corridorTex, corridorTexBlocked, corridorWidth);
        DrawKit::Rooms(g, pos, treasure, roomTex, treasureTex, roomSize);


        DrawKit::Graph(g, pos, treasure);
        drawHintPath();
        drawNeighborHighlights();
        DrawKit::Players(p1,p2,pos);
        // Dibujo de los sprites de los jugadores
        auto drawAnim = [&](Texture2D tex, const SpriteAnim& a, Vector2 wp){
            if (tex.id == 0) return;
            float cw = (float)tex.width  / a.cols;
            float ch = (float)tex.height / a.rows;
            Rectangle src = { a.frame * cw, a.row * ch, cw, ch };
            Rectangle dst = { wp.x, wp.y, playerSpriteW, playerSpriteH };
            Vector2 origin = { playerSpriteW*0.5f, playerSpriteH*0.5f };
            DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
        };



        drawPlayerSprite(p1Tex, pos[p1.node], playerSpriteW, playerSpriteH);
        drawPlayerSprite(p2Tex, pos[p2.node], playerSpriteW, playerSpriteH);

        EndMode2D();

        // HUD
        DrawRectangle(0,0,W,80,(Color){18,18,22,255});
        DrawText("Cazador de Tesoro ",20,14,24,RAYWHITE);
        DrawText(TextFormat("Nivel: %d  |  Turnos: %d/%d",levelIndex,turnCount,cfg.maxTurns),20,46,18,GRAY);
        DrawText(TextFormat("Tesoro en nodo %d",treasure),460,46,18,GOLD);
        DrawText(TextFormat("J1: nodo %d  puntaje %d",p1.node,p1.score),W-500,14,18,p1.color);
        DrawText(TextFormat("J2: nodo %d  puntaje %d",p2.node,p2.score),W-500,40,18,p2.color);
        DrawText(TextFormat("Turno: %s", (currentTurn==1?"J1":"J2")), W-200, 40, 18, RAYWHITE);

        DrawText("WASD/Flechas camara | Rueda/Q/E zoom | Click en vecino para moverte. (Trampas cambian cada turno)", 20, H-30, 18, GRAY);
        if(mustPass && !inFight){ DrawText("Sin movimientos: pulsa [ESPACIO] para pasar turno", 20, H-54, 18, ORANGE); }
        if(inFight){ drawFightUI(); }

        if (fightMsgFrames > 0){ int tw = MeasureText(fightMsg, 20); DrawRectangle(W/2-(tw+40)/2, 82, tw+40, 32, (Color){16,16,16,220}); DrawText(fightMsg, W/2 - tw/2, 90, 20, ORANGE); fightMsgFrames--; }

        if(someoneWon()){
            const char* wt = winnerText(); int tw = MeasureText(wt,28);
            DrawRectangle(W/2-(tw+40)/2,90,tw+40,50,(Color){16,16,16,230});
            DrawText(wt, W/2-tw/2,105,28, YELLOW);
            DrawText("Presiona [1][2][3] para reiniciar.", W/2-160,140,18, LIGHTGRAY);
        }

        EndDrawing();
    }

    // NUEVO: descargar texturas
    if (p2Tex.id != 0) UnloadTexture(p2Tex);
    if (p1Tex.id != 0) UnloadTexture(p1Tex);

    if (corridorTexBlocked.id != 0) UnloadTexture(corridorTexBlocked);
    if (corridorTex.id != 0)        UnloadTexture(corridorTex);
    if (treasureTex.id != 0) UnloadTexture(treasureTex);
    if (roomTex.id != 0) UnloadTexture(roomTex);
    if (menuBgTex.id != 0) UnloadTexture(menuBgTex);

    CloseWindow();
}



