#ifndef STRINGREQUESTHANDLER_H
#define STRINGREQUESTHANDLER_H

#include <string>
#include <functional>
#include "httprequest.h"
#include "httpresponse.h"

class StringRequestHandler : public web::RequestHandler
{
    std::function<void (const std::string&)> _logging;
    std::string _message;
public:
    StringRequestHandler(const std::string& message);
    virtual ~StringRequestHandler();

    void SetLogging(std::function<void (const std::string&)> logging);

    virtual int ConstructResponse(const web::Request &request, web::Response &response) const;

};

#endif // STRINGREQUESTHANDLER_H
