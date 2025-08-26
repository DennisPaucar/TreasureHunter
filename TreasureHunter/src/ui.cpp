#include "../include/ui.hpp"

void CameraUI::init(int w,int h){
    W=w; H=h; cam = {0}; cam.offset={(float)W/2,(float)H/2}; cam.zoom=1.0f; cam.target={0,0};
}

void CameraUI::controls(float dt){
    float sp = 600.0f * dt;
    if(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))   cam.target.y -= sp;
    if(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN)) cam.target.y += sp;
    if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT)) cam.target.x -= sp;
    if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))cam.target.x += sp;

    float wheel = GetMouseWheelMove(); if(wheel!=0){
        float factor = 1.0f + wheel*0.1f;
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
        cam.zoom *= factor; if(cam.zoom<0.2f) cam.zoom=0.2f; if(cam.zoom>3.5f) cam.zoom=3.5f;
        Vector2 after = GetScreenToWorld2D(GetMousePosition(), cam);
        cam.target.x += (mouseWorld.x - after.x); cam.target.y += (mouseWorld.y - after.y);
    }
    if(IsKeyDown(KEY_Q)) cam.zoom *= (1.0f+dt); if(IsKeyDown(KEY_E)) cam.zoom *= (1.0f-dt);
    if(cam.zoom<0.2f) cam.zoom=0.2f; if(cam.zoom>3.5f) cam.zoom=3.5f;

    static bool dragging=false; static Vector2 prev={0,0};
    if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){ dragging=true; prev = GetMousePosition(); }
    if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)&&dragging){ Vector2 m=GetMousePosition(); Vector2 delta={ (m.x-prev.x)/cam.zoom, (m.y-prev.y)/cam.zoom }; cam.target.x -= delta.x; cam.target.y -= delta.y; prev=m; }
    if(IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) dragging=false;
}

void CameraUI::onResize(int w,int h){
    W=w; H=h;
    cam.offset = { (float)W/2, (float)H/2 };
}

namespace DrawKit {

static void DrawSpriteCentered(Texture2D tex, Vector2 center, float size, float rotDeg=0.0f) {
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {center.x, center.y, size, size};
    Vector2 origin = {size/2.0f, size/2.0f};
    DrawTexturePro(tex, src, dst, origin, rotDeg, WHITE);
}

void Graph(const ::Graph& g, const Vector2 pos[MAXN], int treasure){
    for(int u=0;u<g.n;++u){
        for(int i=0;i<g.deg[u];++i){ Edge e=g.adj[u][i]; if(u<e.to){
            Color c=e.blocked?(Color){180,60,60,170}:(Color){0,0,0,0};
            DrawLineEx(pos[u], pos[e.to], e.blocked?2.0f:3.0f, c);
            if(e.blocked){ Vector2 m={(pos[u].x+pos[e.to].x)/2.0f,(pos[u].y+pos[e.to].y)/2.0f}; DrawCircleV(m,6,RED); DrawText("X", (int)m.x-5,(int)m.y-9,14,BLACK);} }
        }
    }
    for(int i=0;i<g.n;++i){
        Color fill= (i == treasure) ? (Color){0, 0, 0, 80} : RAYWHITE; Color outline=(i==treasure)?(Color){0, 0, 0, 80}: RAYWHITE;
        DrawCircleV(pos[i],18,fill); DrawCircleLines((int)pos[i].x,(int)pos[i].y,18,outline);
        DrawText(TextFormat("%d",i),(int)pos[i].x-6,(int)pos[i].y-7,16,BLACK);
        if(i==treasure) DrawText("$",(int)pos[i].x-5,(int)pos[i].y-28,20,DARKBROWN);
    }
}

void Token(const Player& p, const Vector2 pos[MAXN]){
    DrawCircleV(pos[p.node],18,p.color);
    DrawCircleLines((int)pos[p.node].x,(int)pos[p.node].y,12,BLACK);
}

void Players(const Player& p1, const Player& p2, const Vector2 pos[MAXN]){
    Token(p1,pos); Token(p2,pos);
}

void Rooms(const ::Graph& g, const Vector2 pos[MAXN], int treasure,
           Texture2D roomTex, Texture2D treasureTex, float roomSize){
    bool hasRoom = roomTex.id != 0;
    bool hasTre  = treasureTex.id != 0;

    for (int i = 0; i < g.n; ++i) {
        Texture2D texToUse = roomTex;
        if (i == treasure && hasTre) texToUse = treasureTex;
        if (texToUse.id != 0) {
            DrawSpriteCentered(texToUse, pos[i], roomSize, 0.0f);
        }
        // Si quieres mantener overlay dorado además de la textura especial:
        if (i == treasure) {
            DrawCircleV(pos[i], roomSize / 1.5f, (Color){255,215,0,30});
        }

    }
}

// --- Corredor "tileado": repite el sprite a lo largo de la arista sin deformarlo ---
static void DrawCorridorTiled(Texture2D tex, Vector2 a, Vector2 b, float width, Color tint){
    if (tex.id == 0) return;

    // Dirección y longitud
    Vector2 d = { b.x - a.x, b.y - a.y };
    float len = sqrtf(d.x*d.x + d.y*d.y);
    if (len <= 0.0001f) return;

    // Ángulo en grados para DrawTexturePro
    float angDeg = atan2f(d.y, d.x) * 180.0f / PI;

    // Mantener relación de aspecto del sprite:
    // si el sprite es texW×texH, al dibujar alto=width, el "largo natural" de un módulo es:
    float texW = (float)tex.width;
    float texH = (float)tex.height;
    float moduleLen = texW * (width / texH);

    // Densidad de repetición opcional (1.0 = aspecto puro)
    const float density = 1.5f;          // <--- ajusta si quieres más/menos repeticiones
    moduleLen *= density;

    if (moduleLen < 1.0f) moduleLen = 1.0f; // evita loops gigantes

    float placed = 0.0f;
    while (placed < len) {
        float remaining = len - placed;
        float segLen = (remaining < moduleLen) ? remaining : moduleLen;

        // recorte de fuente si el último tramo es parcial
        float u = segLen / moduleLen;       // [0..1]
        float srcW = texW * u;

        // centro del tramo actual
        float tmid = placed + segLen * 0.5f;
        Vector2 mid = { a.x + (d.x/len)*tmid, a.y + (d.y/len)*tmid };

        // rectángulos source/dest
        Rectangle src = { 0, 0, srcW, texH };
        Rectangle dst = { mid.x, mid.y, segLen, width };
        Vector2 origin = { segLen/2.0f, width/2.0f };

        DrawTexturePro(tex, src, dst, origin, angDeg, tint);

        placed += segLen;
    }
}

void Corridors(const ::Graph& g, const Vector2 pos[MAXN],
               Texture2D corridorTexOpen,
               Texture2D corridorTexBlocked,
               float corridorWidth)
{
    if (corridorTexOpen.id == 0 && corridorTexBlocked.id == 0) return;

    for (int u = 0; u < g.n; ++u) {
        for (int i = 0; i < g.deg[u]; ++i) {
            Edge e = g.adj[u][i];
            if (u < e.to) {
                Texture2D tex = e.blocked ? corridorTexBlocked : corridorTexOpen;

                if (tex.id != 0) {
                    DrawCorridorTiled(tex, pos[u], pos[e.to], corridorWidth, WHITE);
                } else {
                    // Fallback: si falta la textura específica, usa la otra con tinte
                    Texture2D alt = e.blocked ? corridorTexOpen : corridorTexBlocked;
                    if (alt.id != 0) {
                        Color tint = e.blocked ? (Color){200,80,80,200} : WHITE;
                        DrawCorridorTiled(alt, pos[u], pos[e.to], corridorWidth, tint);
                    }
                }
            }
        }
    }
}




} // namespace DrawKit
