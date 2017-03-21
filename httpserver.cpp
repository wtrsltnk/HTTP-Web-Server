#include "httpserver.h"
#include <iostream>
#include <sstream>
#include <thread>
#include "httprequest.h"
#include "httpresponse.h"

using namespace web;

// Global Functions
std::string  ToString(int data)
{
    std::stringstream buffer;
    buffer << data;
    return buffer.str();
}

// Constructor
HttpServer::HttpServer(int port)
    : _logging([] (const std::string& message) { std::cout << message << std::endl; })
{
    // Socket Settings
    ZeroMemory(&_hints, sizeof(_hints));
    _hints.ai_family = AF_INET;
    _hints.ai_flags = AI_PASSIVE;
    _hints.ai_socktype = SOCK_STREAM;
    _hints.ai_protocol = IPPROTO_TCP;

    // Initialize Data
    _maxConnections = 50;
    _listeningSocket = INVALID_SOCKET;
    _socketVersion = MAKEWORD(2,2);
    _port = ToString(port);
}

void HttpServer::SetLogging(std::function<void(const std::string&)> logging)
{
    this->_logging = logging;
}

// Main Functions
void HttpServer::Init()
{
    // Start Winsocket
    auto resultCode = WSAStartup(_socketVersion, &_wsaData);
    if(0 != resultCode)
    {
        this->_logging("Initialize Failed \nError Code : " + ToString(resultCode));
        ErrorQuit();
    }
}

void HttpServer::Start(std::function<Response (const Request)> onConnection)
{
    // Resolve Local Address And Port
    auto resultCode = getaddrinfo(NULL, _port.c_str(), &_hints, &_result);
    if (0 != resultCode)
    {
        this->_logging("Resolving Address And Port Failed \nError Code: " + ToString(resultCode));
        ErrorQuit();
    }

    // Create Socket
    _listeningSocket = socket(_hints.ai_family, _hints.ai_socktype, _hints.ai_protocol);
    if (INVALID_SOCKET == _listeningSocket)
    {
        this->_logging("Could't Create Socket");
        ErrorQuit();
    }

    // Bind
    resultCode = bind(_listeningSocket, _result->ai_addr, (int)_result->ai_addrlen);
    if (SOCKET_ERROR == resultCode)
    {
        this->_logging("Bind Socket Failed");
        ErrorQuit();
    }

    // Listen
    resultCode = listen(_listeningSocket, _maxConnections);
    if (SOCKET_ERROR == resultCode)
    {
        this->_logging("Listening On Port " + _port + " Failed");
        ErrorQuit();
    }
    else
    {
        this->_logging("-Server Is Up And Running.\n--Listening On Port " + _port + "...");
    }

    while (true)
    {
        sockaddr_in clientInfo;
        int clientInfoSize = sizeof(clientInfo);

        auto socket = accept(_listeningSocket, (sockaddr*)&clientInfo, &clientInfoSize);
        if (INVALID_SOCKET == socket)
        {
            this->_logging("Accepting Connection Failed");
        }
        else
        {
            this->_logging("Spinning thread to handle request");

            std::thread t(Request::handleRequest, onConnection, Request(socket, clientInfo));
            t.detach();
        }
    }
}

void HttpServer::Stop()
{
    closesocket(_listeningSocket);
    WSACleanup();
}

void HttpServer::ErrorQuit()
{
    Stop();
    this->_logging("An Error Occurred.");
}
