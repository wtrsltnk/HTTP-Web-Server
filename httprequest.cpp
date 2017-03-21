#include "httprequest.h"
#include "httpresponse.h"
#include <algorithm>
#include <sstream>

using namespace web;

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

std::string Request::getMessage()
{
    char buffer[BUFFER_SIZE+1];
    int bytes;

    bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes] = '\0';
    std::string browserData = buffer;

    while (BUFFER_SIZE == bytes)
    {
        bytes = recv(this->_socket, buffer, BUFFER_SIZE, 0);
        buffer[bytes] = '\0';
        browserData += buffer;
    }

    return browserData;
}

// The function we want to execute on the new thread.
void Request::handleRequest(std::function<Response (const Request)> onConnection, Request request)
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

std::string Request::ipAddress() const
{
    return std::string(inet_ntoa(this->_clientInfo.sin_addr));
}
