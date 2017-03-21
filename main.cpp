#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
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

    virtual web::Response ConstructResponse(const web::Request& request);

};

StringRequestHandler::StringRequestHandler(const std::string& message) : _message(message) { }

StringRequestHandler::~StringRequestHandler() { }

web::Response StringRequestHandler::ConstructResponse(const web::Request &request)
{
    web::Response response;

    response._response += this->_message;

    return response;
}

class FileSystemRequestHandler : public web::RequestHandler
{
    System::IO::DirectoryInfo _root;
public:
    FileSystemRequestHandler(const std::string& root);
    virtual ~FileSystemRequestHandler();

    virtual web::Response ConstructResponse(const web::Request& request);
};

FileSystemRequestHandler::FileSystemRequestHandler(const std::string& root)
    : _root(System::IO::DirectoryInfo(root))
{
    std::cout << this->_root.FullName() << std::endl;
}

FileSystemRequestHandler::~FileSystemRequestHandler() { }

web::Response FileSystemRequestHandler::ConstructResponse(const web::Request &request)
{
    web::Response response;

    auto uri = request._uri;
    if (uri[0] == '/') uri = uri.substr(1);

    auto fullPath = System::IO::Path::Combine(this->_root.FullName(), uri);

    std::cout << fullPath << std::endl;

    std::stringstream ss;

    ss << "<html><body>";

    if (System::IO::DirectoryInfo(fullPath).Exists())
    {
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
    }
    else if (System::IO::FileInfo(fullPath).Exists())
    {
    }
    else
    {
        ss << request._uri << " does not exist";
    }

    ss << "</body></html>";

    response._response += ss.str();

    return response;
}

int main(int argc,char*argv[])
{
    auto server = web::HttpServer(8080);

    server.SetLogging([] (const std::string& message) {
        std::cout << message << std::endl;
    });

    server.Init();

    server.Start([] (const web::Request request) -> web::Response {
        std::cout << "Request recieved from " << request.ipAddress()
                  << " for " << request._uri << std::endl;

        FileSystemRequestHandler handler("C:\\temp");
        return handler.ConstructResponse(request);
    });

	return 0;
}
