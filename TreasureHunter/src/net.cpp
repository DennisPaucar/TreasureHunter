// net.cpp: Implementacion de los sockets y potocoloss
#include "../include/net.hpp"
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib,"ws2_32.lib")
  using socklen_t = int;
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <netdb.h>
  #define closesocket close
#endif

NetClient::NetClient(){
#ifdef _WIN32
    WSADATA w; if(WSAStartup(MAKEWORD(2,2), &w)==0) wsaInited=true;
#endif
}
NetClient::~NetClient(){ disconnect();
#ifdef _WIN32
    if(wsaInited) WSACleanup();
#endif
}

bool NetClient::connectTo(const std::string& host, uint16_t port){
    disconnect();
    sock = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (sock<0) return false;

    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0){
        // resolver DNS
        hostent* he = gethostbyname(host.c_str());
        if (!he){ closesocket(sock); sock=-1; return false; }
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    if (connect(sock, (sockaddr*)&addr, sizeof(addr))<0){
        closesocket(sock); sock=-1; return false;
    }
    connected = true;
    running = true;
    rxThread = std::thread(&NetClient::rxLoop, this);
    return true;
}
void NetClient::disconnect(){
    running = false;
    if (sock!=-1){ shutdown(sock, 2); closesocket(sock); sock=-1; }
    if (rxThread.joinable()) rxThread.join();
    connected = false;
}

bool NetClient::sendFrame(const std::vector<uint8_t>& buf){
    if (!connected) return false;
    uint32_t n = (uint32_t)buf.size();
    uint32_t n_be = htonl(n);
    if (send(sock,(const char*)&n_be,4,0)!=4) return false;
    size_t off=0;
    while (off<buf.size()){
        int s = (int)send(sock,(const char*)buf.data()+off,(int)(buf.size()-off),0);
        if (s<=0) return false;
        off += (size_t)s;
    }
    return true;
}
bool NetClient::sendText(const std::string& s){
    std::vector<uint8_t> b(s.begin(), s.end());
    return sendFrame(b);
}
bool NetClient::recvExact(void* out, size_t n){
    char* p=(char*)out; size_t got=0;
    while (got<n){
        int r = (int)recv(sock,p+(int)got,(int)(n-got),0);
        if (r<=0) return false; got += (size_t)r;
    }
    return true;
}
bool NetClient::recvFrame(std::vector<uint8_t>& out){
    uint32_t n_be; if(!recvExact(&n_be,4)) return false;
    uint32_t n = ntohl(n_be);
    out.resize(n);
    if (n>0 && !recvExact(out.data(), n)) return false;
    return true;
}
void NetClient::rxLoop(){
    while (running){
        std::vector<uint8_t> f;
        if(!recvFrame(f)) break;
        NetMessage m; m.text.assign(f.begin(), f.end());
        {
            std::lock_guard<std::mutex> lk(qmtx);
            q.push(std::move(m));
        }
    }
    connected=false;
}
bool NetClient::poll(NetMessage& out){
    std::lock_guard<std::mutex> lk(qmtx);
    if (q.empty()) return false;
    out = std::move(q.front()); q.pop();
    return true;
}
