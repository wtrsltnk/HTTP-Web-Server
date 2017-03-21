#ifndef HTPPSERVER_H
#define HTPPSERVER_H

#include <functional>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT 8080

namespace web
{

class HttpServer
{
    std::function<void (const std::string&)> _logging;
public:
    HttpServer(int port = DEFAULT_PORT);
    void SetLogging(std::function<void (const std::string&)> logging);

    void Init(); // Initialize Server
    void Start(std::function<class Response (const class Request)> onConnection); // Start server
    void Stop(); // Close Server
    void ErrorQuit(); // Quit And Display An Error Message

    static void handleRequest(std::function<class Response (const class Request)> onConnection, class Request request);

private:
    WORD _socketVersion;
    WSADATA _wsaData;
    SOCKET _listeningSocket;
    addrinfo _hints,*_result;
    int _maxConnections;

    //  Server Info
   std::string _port;

};

}

#endif // HTPPSERVER_H
