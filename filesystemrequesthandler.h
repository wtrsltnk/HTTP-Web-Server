#ifndef FILESYSTEMREQUESTHANDLER_H
#define FILESYSTEMREQUESTHANDLER_H

#include <string>
#include <functional>
#include <system.io.directoryinfo.h>
#include "mimetypes.h"
#include "httprequest.h"
#include "httpresponse.h"

class FileSystemRequestHandler : public web::RequestHandler
{
    std::function<void (const std::string&)> _logging;
    System::IO::DirectoryInfo _root;
    MimeTypes _mimeTypes;
public:
    FileSystemRequestHandler();
    virtual ~FileSystemRequestHandler();

    void SetRoot(const std::string& root);
    void SetLogging(std::function<void (const std::string&)> logging);
    MimeTypes &GetMimeTypes();

    virtual int ConstructResponse(const web::Request &request, web::Response &response) const;
};

#endif // FILESYSTEMREQUESTHANDLER_H
