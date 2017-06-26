#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <map>

#include "httprequest.h"

namespace web
{

class Response
{
public:
    Response();
    virtual ~Response();

    void addHeader(const std::string& key, const std::string& value);

    int _responseCode;
    std::map<std::string, std::string> _headers;
    std::string _response;
};

}

#endif // HTTPRESPONSE_H
