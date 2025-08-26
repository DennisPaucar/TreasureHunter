// net.hpp : Proceso de protocolos para la VM
#pragma once
#include <string>
#include <vector>
#include <thread>//funciones para los protocolos
#include <atomic>
#include <mutex>
#include <queue>
#include <cstdint>

struct NetMessage {
    std::string text; //Serializacion
};

class NetClient {
public:
    NetClient();
    ~NetClient();

    bool connectTo(const std::string& host, uint16_t port);
    void disconnect();
    bool isConnected() const { return connected; }

    bool sendText(const std::string& s);
    bool poll(NetMessage& out);

private:
    int sock = -1;
    std::thread rxThread;
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    std::mutex qmtx;
    std::queue<NetMessage> q;

    void rxLoop();

    bool sendFrame(const std::vector<uint8_t>& buf);
    bool recvExact(void* out, size_t n);
    bool recvFrame(std::vector<uint8_t>& out);

#ifdef _WIN32
    bool wsaInited=false;
#endif
};

