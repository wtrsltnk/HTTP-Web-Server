#ifndef _SERVER_H
#define _SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <fstream>
#include <functional>

#define AppVersion "1.0beta"
#define DEFAULT_PORT "8080"
#define BUFFER_SIZE 1024*5 // 5KB
#define ACC_EXT_NUM 48
#define OS_TYPE_NUM 9

using std::string;

extern string ToString(int data);

class Request
{
    SOCKET _socket;
    string getMessage();
public:
    Request(SOCKET socket, sockaddr_in clientInfo);

    static void handleRequest(std::function<class Response (Request)> onConnection, Request request);

    sockaddr_in _clientInfo;
    std::string _method;
    std::string _uri;
    std::map<std::string, std::string> _headers;
    std::string _payload;
};

class Response
{
public:
    Response(Request& request);

    std::string _response;
};

class RequestHandler
{
public:
    virtual ~RequestHandler() { }

    virtual Response ConstructResponse(Request& request) = 0;
};

class MessageRequestHandler : public RequestHandler
{
    std::string _message;
public:
    MessageRequestHandler(const std::string& message);
    virtual ~MessageRequestHandler();

    virtual Response ConstructResponse(Request& request);

};

class HttpServer
{
    std::function<void (const std::string&)> _logging;
public:
    HttpServer(std::function<void (const std::string&)> logging);

    void Init(); // Initialize Server
    void Start(std::function<Response (Request)> onConnection); // Start server
    void Stop(); // Close Server
    void ErrorQuit(); // Quit And Display An Error Message

private:
    WORD _socketVersion;
    WSADATA _wsaData;
    SOCKET _listeningSocket;
    addrinfo _hints,*_result;
    int _maxConnections;

    //  Server Info
    string _port;

};

#endif

