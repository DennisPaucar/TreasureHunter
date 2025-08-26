#include "../include/graph.hpp"

void Graph::init(int n_){ n=n_; for(int i=0;i<MAXN;++i) deg[i]=0; }

bool Graph::existsEdge(int u,int v) const {
    for(int i=0;i<deg[u];++i) if(adj[u][i].to==v) return true; return false;
}

void Graph::addUndirected(int u,int v,float w){
    if(u==v||existsEdge(u,v)) return;
    if(deg[u]<MAXDEG&&deg[v]<MAXDEG){ adj[u][deg[u]++]={v,w,false}; adj[v][deg[v]++]={u,w,false}; }
}

int Graph::findEdgeIdx(int u,int v){ for(int i=0;i<deg[u];++i) if(adj[u][i].to==v) return i; return -1; }

void Graph::shuffleTraps(float p){
    int th=(int)(p*1000.0f);
    for(int u=0;u<n;++u){
        for(int i=0;i<deg[u];++i){
            int v=adj[u][i].to; if(u<v){
                bool b=(GetRandomValue(0,999)<th);
                adj[u][i].blocked=b; int j=findEdgeIdx(v,u); if(j!=-1) adj[v][j].blocked=b;
            }
        }
    }
}

void Graph::dijkstra(int src, float dist[MAXN], int parent[MAXN], bool ignoreBlocked) const {
    struct LocalHeap {
        float hD[MAXHEAP];
        int   hN[MAXHEAP];
        int   hs;

        void init() { hs = 0; }

        void swapIdx(int a, int b) {
            float td = hD[a]; hD[a] = hD[b]; hD[b] = td;
            int   tn = hN[a]; hN[a] = hN[b]; hN[b] = tn;
        }

        void up(int i) {
            while (i > 0) {
                int p = (i - 1) / 2;
                if (hD[p] <= hD[i]) break;
                swapIdx(p, i);
                i = p;
            }
        }

        void down(int i) {
            for (;;) {
                int l = 2*i + 1;
                int r = l + 1;
                int sm = i;
                if (l < hs && hD[l] < hD[sm]) sm = l;
                if (r < hs && hD[r] < hD[sm]) sm = r;
                if (sm == i) break;
                swapIdx(i, sm);
                i = sm;
            }
        }

        void push(float d, int v) {
            if (hs >= MAXHEAP) return;
            hD[hs] = d;
            hN[hs] = v;
            up(hs);
            hs++;
        }

        void pop(float &d, int &v) {
            d = hD[0];
            v = hN[0];
            hs--;
            if (hs > 0) {
                hD[0] = hD[hs];
                hN[0] = hN[hs];
                down(0);
            }
        }

        bool empty() const { return hs == 0; }
    };

    const float INF = std::numeric_limits<float>::infinity();
    bool used[MAXN];
    for (int i = 0; i < n; ++i) {
        dist[i] = INF;
        parent[i] = -1;
        used[i] = false;
    }

    LocalHeap heap;
    heap.init();

    dist[src] = 0.0f;
    heap.push(0.0f, src);

    while (!heap.empty()) {
        float d; int u;
        heap.pop(d, u);
        if (used[u]) continue;
        used[u] = true;
        if (d > dist[u]) continue;

        for (int i = 0; i < deg[u]; ++i) {
            const Edge &e = adj[u][i];
            if (!ignoreBlocked && e.blocked) continue;
            int v = e.to;
            float nd = d + e.w;
            if (nd < dist[v]) {
                dist[v] = nd;
                parent[v] = u;
                heap.push(nd, v);
            }
        }
    }
}

void Graph::dijkstraAllEdges(int src, float dist[MAXN], int parent[MAXN]) const { dijkstra(src, dist, parent, true); }

int Graph::neighborsOpen(int u,int out[MAXDEG]) const { int k=0; for(int i=0;i<deg[u];++i) if(!adj[u][i].blocked) out[k++]=adj[u][i].to; return k; }
