// netplay.hpp
#pragma once
#include "net.hpp"
#include <string>
#include <functional>
#include <sstream>
#include <vector>


struct NetPlay {
    bool enabled=false;
    bool isHost=false;  // J1 host, J2 join
    NetClient cli;

    // callbacks para aplicar en Game
    std::function<void(int level, unsigned seed)> onSetup;
    std::function<void(int who, int to)>          onMove;
    std::function<void(int who)>                  onPass;
    std::function<void(int mover, int node)>      onFightStart;
    std::function<void(int winner)>               onFightRes;
    std::function<void(/*...state args...*/const std::string& stateLine)> onStateLine;
    std::function<void(const std::vector<std::pair<int,int>>&)> onBlockedList;

    bool isConnected() const { return enabled && cli.isConnected(); }

    void disconnect(){
        if (!enabled) return;
        cli.disconnect();
        enabled = false;
    }


    bool connect(const std::string& ip, uint16_t port, bool host){
        if (!cli.connectTo(ip, port)) return false;
        enabled = true; isHost = host;
        cli.sendText(std::string("HELLO role=") + (host?"HOST":"JOIN"));
        return true;
    }

    void sendSetup(int lvl, unsigned seed){
        if (!enabled || !isHost) return;
        cli.sendText("SETUP level=" + std::to_string(lvl) + " seed=" + std::to_string(seed));
    }
    void sendMove(int who, int to){
        if (!enabled) return;
        cli.sendText("MOVE who=" + std::to_string(who) + " to=" + std::to_string(to));
    }
    void sendPass(int who){
        if (!enabled) return;
        cli.sendText("PASS who=" + std::to_string(who));
    }
    void sendFightStart(int mover, int node){
        if (!enabled || !isHost) return;
        cli.sendText("FIGHT start mover=" + std::to_string(mover) + " node=" + std::to_string(node));
    }
    void sendFightRes(int winner){
        if (!enabled || !isHost) return;
        cli.sendText("FIGHTRES winner=" + std::to_string(winner));
    }
    void sendStateLine(const std::string& s){
        if (!enabled || !isHost) return;
        cli.sendText(std::string("STATE ")+s);
    }

    void sendBlockedList(const std::vector<std::pair<int,int>>& bl){
        if (!enabled || !isHost) return;
        // Formato: "BLOCKS <k> u0,v0 u1,v1 u2,v2 ..."
        std::string s = "BLOCKS ";
        s += std::to_string((int)bl.size());
        for (auto &p : bl){
            s.push_back(' ');
            s += std::to_string(p.first);
            s.push_back(',');
            s += std::to_string(p.second);
        }
        cli.sendText(s);
    }


    // Llamar cada frame desde Game::run()
    void pump(){
        if(!enabled) return;
        NetMessage m;
        while (cli.poll(m)){
            const std::string& t = m.text;
            // parse muy simple por prefijo
            if      (t.rfind("SETUP",0)==0){
                int lvl=1; unsigned seed=12345;
                sscanf(t.c_str(),"SETUP level=%d seed=%u",&lvl,&seed);
                if(onSetup) onSetup(lvl,seed);
            } else if (t.rfind("MOVE",0)==0){
                int who=1,to=0; sscanf(t.c_str(),"MOVE who=%d to=%d",&who,&to);
                if(onMove) onMove(who,to);
            } else if (t.rfind("PASS",0)==0){
                int who=1; sscanf(t.c_str(),"PASS who=%d",&who);
                if(onPass) onPass(who);
            } else if (t.rfind("FIGHTRES",0)==0){
                int w=0; sscanf(t.c_str(),"FIGHTRES winner=%d",&w);
                if(onFightRes) onFightRes(w);
            } else if (t.rfind("FIGHT start",0)==0){
                int mover=1,node=0; sscanf(t.c_str(),"FIGHT start mover=%d node=%d",&mover,&node);
                if(onFightStart) onFightStart(mover,node);
            } else if (t.rfind("STATE",0)==0){
                if(onStateLine) onStateLine(t.substr(6)); // resto
            }
             else if (t.rfind("BLOCKS",0)==0){
                std::istringstream iss(t);
                std::string cmd; int k=0;
                iss >> cmd >> k; // cmd="BLOCKS"
                std::vector<std::pair<int,int>> bl; bl.reserve(k);
                std::string tok;
                while (iss >> tok){
                    auto c = tok.find(',');
                    if (c!=std::string::npos){
                        int u = std::stoi(tok.substr(0, c));
                        int v = std::stoi(tok.substr(c+1));
                        bl.emplace_back(u,v);
                    }
                }
                if (onBlockedList) onBlockedList(bl);
            }
        }
    }
};


