#ifndef HELPREQUESTHANDLER_H
#define HELPREQUESTHANDLER_H

#include "httprequest.h"

class HelpRequestHandler : public web::RequestHandler
{
public:
    HelpRequestHandler();

    virtual int ConstructResponse(const web::Request &request, web::Response &response) const;
};

#endif // HELPREQUESTHANDLER_H
