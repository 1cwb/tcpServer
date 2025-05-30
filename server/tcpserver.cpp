#include "tcpserver.hpp"

using namespace std;

#define MAX_EVENTS 1024
#define PORT 8080
#define WORKER_THREADS 4

int main(int argc, char** argv)
{
    TcpServer server(PORT, WORKER_THREADS);
    server.setConnectedCallback([](TcpServer::ptrConnection conn) {
        cout << "connected: " << conn->getFd() << endl;
    });
    server.setCloseCallback([](TcpServer::ptrConnection conn) {
        cout << "disconnected: " << conn->getFd() << endl;
    });
    server.setMessageCallback([](TcpServer::ptrConnection conn, Buffer* buffer) {
        cout << "message: " << conn->getFd()  << " " << buffer->readAsString(buffer->readableSize()) << endl;
    });
    server.start();
    while(1)
    {
        sleep(1);
    }
    return 0;
}