#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httputility.h"
#include <system.io.directoryinfo.h>
#include <system.io.fileinfo.h>
#include <system.io.path.h>

using namespace std;

class StringRequestHandler : public web::RequestHandler
{
    std::string _message;
public:
    StringRequestHandler(const std::string& message);
    virtual ~StringRequestHandler();

    virtual int ConstructResponse(const web::Request &request, web::Response &response);

};

StringRequestHandler::StringRequestHandler(const std::string &message) : _message(message) { }

StringRequestHandler::~StringRequestHandler() { }

int StringRequestHandler::ConstructResponse(const web::Request &request, web::Response &response)
{
    response._response += this->_message;

    return 200;
}

class FileSystemRequestHandler : public web::RequestHandler
{
    System::IO::DirectoryInfo _root;
public:
    FileSystemRequestHandler(const std::string& root);
    virtual ~FileSystemRequestHandler();

    virtual int ConstructResponse(const web::Request &request, web::Response &response);
};

FileSystemRequestHandler::FileSystemRequestHandler(const std::string& root)
    : _root(System::IO::DirectoryInfo(root))
{
    std::cout << this->_root.FullName() << std::endl;
}

FileSystemRequestHandler::~FileSystemRequestHandler() { }

int FileSystemRequestHandler::ConstructResponse(const web::Request &request, web::Response& response)
{
    auto uri = request._uri;
    if (uri[0] != '/')
    {
        return 500;
    }

    uri = uri.substr(1);

    uri = url_decode(uri);

    auto fullPath = System::IO::Path::Combine(this->_root.FullName(), uri);

    std::cout << fullPath << std::endl;

    for (auto header : request._headers) std::cout << "[header] " << header.first << ": " << header.second << "\n";

    std::stringstream ss;

    if (System::IO::DirectoryInfo(fullPath).Exists())
    {
        ss << "<html><body>";

        System::IO::DirectoryInfo directory(fullPath);
        std::cout << "directory.FullName()           " << directory.FullName() << std::endl;
        std::cout << "directory.Parent().FullName()  " << directory.Parent().FullName() << std::endl;

        ss << "<h1>" << request._uri << "</h1>";
        ss << "<h2>Directories:</h2>";
        ss << "<ul>";
        if (request._uri != "/")
        {
            auto relativeParentPath = directory.Parent().FullName().substr(this->_root.FullName().size());
            std::cout << "relativeParentPath             " << relativeParentPath << std::endl;
            if (relativeParentPath == "") relativeParentPath = "/";
            ss << "<li><a href=\"" << relativeParentPath << "\">..</a></li>";
        }
        for (auto dir : directory.GetDirectories())
        {
            System::IO::DirectoryInfo dirInfo(dir);
            auto relativePath = dirInfo.FullName().substr(this->_root.FullName().size());
            ss << "<li><a href=\"" << relativePath << "\">" << dirInfo.Name() << "</a></li>";
        }
        ss << "</ul>";

        ss << "<h2>Files:</h2>";
        ss << "<ul>";
        for (auto file : directory.GetFiles())
        {
            System::IO::FileInfo fileInfo(file);
            auto relativePath = fileInfo.FullName().substr(this->_root.FullName().size());
            ss << "<li><a href=\"" << relativePath << "\">" << fileInfo.Name() << "</a></li>";
        }
        ss << "</ul>";

        ss << "</body></html>";

        response.addHeader("Content-Type", "text/html");
    }
    else if (System::IO::FileInfo(fullPath).Exists())
    {
        ifstream file;
        auto extpos = fullPath.find_last_of('.');
        auto ext = extpos != std::string::npos ? fullPath.substr(extpos) : "";

        if (ext == ".json")
        {
            file = ifstream(fullPath);
            response.addHeader("Content-Type", "application/json");
        }
        else if (ext == ".jpg")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "image/jpeg");
        }
        else if (ext == ".mp4")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "videos/mp4");
        }
        else if (ext == ".xml")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/xml");
        }
        else if (ext == ".js")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/javascript");
        }
        else if (ext == ".css")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/stylesheet");
        }
        else
        {
            file = ifstream(fullPath);
            response.addHeader("Content-Type", "text/html");
        }

        copy(istreambuf_iterator<char>(file),
             istreambuf_iterator<char>(),
             ostreambuf_iterator<char>(ss));
    }
    else
    {
        ss << "<html><body>";

        ss << request._uri << " does not exist";

        ss << "</body></html>";

        response.addHeader("Content-Type", "text/html");
    }

    response._response += ss.str();

    if (request._headers.find("if-range") != request._headers.end())
    {
        std::cout << "they want ranges\n";
    }
    if (request._headers.find("Range") != request._headers.end())
    {
        auto range = request._headers.at("Range");
        std::regex rangeRegex("bytes=([0-9]+)-([0-9]*)");
        std::smatch str_match_result;
        if (std::regex_match(range, str_match_result, rangeRegex))
        {
            auto start = std::stoi(str_match_result[1]);
            auto end = str_match_result[2] != "" ? std::stoi(str_match_result[2]) : start + 500;

            response._response = response._response.substr(start, end - start);
            response._responseCode = 206;
        }
    }

    return 200;
}

int main(int argc,char*argv[])
{
    auto server = web::HttpServer(8080);

    server.SetLogging([] (const std::string& message) {
        std::cout << message << std::endl;
    });

    System::IO::FileInfo exe(argv[0]);

    server.Init();

    while (!server.Start())
    {
        server.SetPortA(server.Port() + 1);
        if (server.Port() > 9999)
        {
            std::cerr << "No available port to listen on" << std::endl;
            return 1;
        }
    }

    ShellExecute(0, 0, server.LocalUrl().c_str(), 0, 0 , SW_SHOW);

    server.WaitForRequests([exe] (const web::Request request, web::Response & response) -> int {
        std::cout << "Request recieved from " << request.ipAddress()
                  << " for " << request._uri << std::endl;

        FileSystemRequestHandler handler(exe.Directory().FullName());
        return handler.ConstructResponse(request, response);
    });

    return 0;
}
