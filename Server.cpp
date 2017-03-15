#include <sstream>
#include <thread>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Server.h"

using namespace std;

// Global Functions
string  ToString(int data)
{
    std::stringstream buffer;
    buffer << data;
    return buffer.str();
}

// Constructor
HttpServer::HttpServer(std::function<void(const std::string&)> logging)
    : _logging(logging)
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
    _port = DEFAULT_PORT;
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

void HttpServer::Start(std::function<Response (Request)> onConnection)
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
    exit(-1);
}

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

Request::Request(SOCKET socket, sockaddr_in clientInfo)
    : _socket(socket), _clientInfo(clientInfo)
{
    std::string allData = getMessage();
    auto pos = allData.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::stringstream ss;
        ss.str(allData.substr(0, pos));
        std::string line;

        // Determine methode, uri and HTTP version
        if (std::getline(ss, line))
        {
            trim(line);
            auto first = line.find_first_of(' ');
            auto last = line.find_last_of(' ');
            if (first != std::string::npos && last != std::string::npos)
            {
                auto version = line.substr(last);
                if (trim(version) != "HTTP/1.1") throw new std::runtime_error("invalid HTTP version");

                auto method = line.substr(0, first);
                this->_method = trim(method);

                auto uri = line.substr(first, last-first);
                this->_uri = trim(uri);
            }
        }

        // Determine headers
        while (std::getline(ss, line))
        {
            auto pos = line.find_first_of(':');
            if (pos != std::string::npos)
            {
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                this->_headers.insert(std::make_pair(trim(key), trim(value)));
            }
        }

        // And finally determine payload(if any)
        this->_payload = allData.substr(pos + 4);
    }
}

string Request::getMessage()
{
    char buffer[BUFFER_SIZE+1];
    int bytes;

    bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes] = '\0';
    string browserData = buffer;

    while (BUFFER_SIZE == bytes)
    {
        bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
        buffer[bytes] = '\0';
        browserData += buffer;
    }

    return browserData;
}

// The function we want to execute on the new thread.
void Request::handleRequest(std::function<Response (Request)> onConnection, Request request)
{
    Response response = onConnection(request);

    std::stringstream headers;

    headers << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << response._response.size() << "\r\n"
            << "\r\n";

    send(request._socket, headers.str().c_str(), headers.str().size(), 0);
    send(request._socket, response._response.c_str(), response._response.size(), 0);

    shutdown(request._socket, SD_BOTH);
    closesocket(request._socket);
}

Response::Response(Request &request) { }

MessageRequestHandler::MessageRequestHandler(const std::string& message) : _message(message) { }

MessageRequestHandler::~MessageRequestHandler() { }

Response MessageRequestHandler::ConstructResponse(Request &request)
{
    auto response = Response(request);

    response._response += this->_message;

    return response;
}
