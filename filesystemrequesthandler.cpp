#include "filesystemrequesthandler.h"
#include "httputility.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <system.io.path.h>
#include <system.io.fileinfo.h>
#include <regex>

FileSystemRequestHandler::FileSystemRequestHandler()
    : _root(System::IO::DirectoryInfo("")), _logging([] (const std::string& message) { std::cout << message << std::endl; })
{
    _mimeTypes.LoadDefaultMimeTypes();
}

FileSystemRequestHandler::~FileSystemRequestHandler() { }

void FileSystemRequestHandler::SetRoot(const std::string& root)
{
    _root = System::IO::DirectoryInfo(root);
}

void FileSystemRequestHandler::SetLogging(std::function<void(const std::string&)> logging)
{
    this->_logging = logging;
}

MimeTypes &FileSystemRequestHandler::GetMimeTypes()
{
    return _mimeTypes;
}

int FileSystemRequestHandler::ConstructResponse(const web::Request &request, web::Response& response) const
{
    _logging(request._method);

    auto uri = request._uri;
    if (uri[0] != '/')
    {
        return 500;
    }

    uri = uri.substr(1);

    uri = url_decode(uri);

    auto fullPath = System::IO::Path::Combine(this->_root.FullName(), uri);

    _logging(fullPath);

    for (auto header : request._headers)
    {
        std::stringstream sss;
        sss << "[header] " << header.first << ": " << header.second;
        _logging(sss.str());
    }

    std::stringstream ss;

    if (System::IO::DirectoryInfo(fullPath).Exists())
    {
        ss << "<html><body>";

        System::IO::DirectoryInfo directory(fullPath);

        ss << "<h1>" << request._uri << "</h1>";
        ss << "<h2>Directories:</h2>";
        ss << "<ul>";
        if (request._uri != "/")
        {
            auto relativeParentPath = directory.Parent().FullName().substr(this->_root.FullName().size());
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
        std::ifstream file;
        auto extpos = fullPath.find_last_of('.');
        auto ext = extpos != std::string::npos ? fullPath.substr(extpos) : "";

        file = std::ifstream(fullPath, std::ios::binary);
        response.addHeader("Content-Type", _mimeTypes[ext]);

        std::copy(std::istreambuf_iterator<char>(file),
                  std::istreambuf_iterator<char>(),
                  std::ostreambuf_iterator<char>(ss));
    }
    else
    {
        ss << "<html><body>";

        ss << request._uri << " does not exist";

        ss << "</body></html>";

        response.addHeader("Content-Type", "text/html");
    }

    response._response += ss.str();

    if (request._headers.find("Range") != request._headers.end())
    {
        auto range = request._headers.at("Range");
        std::regex rangeRegex("bytes=([0-9]+)-([0-9]*)");
        std::smatch str_match_result;
        if (std::regex_match(range, str_match_result, rangeRegex))
        {
            auto start = std::stoi(str_match_result[1]);
            auto end = str_match_result[2] != "" ? std::stoi(str_match_result[2]) : start + 500;

            std::stringstream contentRange;
            contentRange << "bytes " << start << "-" << (start+500) << "/" << response._response.size();
            response._headers["Content-Range"] = contentRange.str();
            response._response = response._response.substr(start, end - start);
            response._responseCode = 206;
        }
    }

    return 200;
}
