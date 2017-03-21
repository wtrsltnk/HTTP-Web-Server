#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>

#include "httprequest.h"

namespace web
{

class Response
{
public:
    Response();
    virtual ~Response();

    std::string _response;
};

}

#endif // HTTPRESPONSE_H
