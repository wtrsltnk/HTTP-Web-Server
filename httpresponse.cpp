#include "httpresponse.h"

using namespace web;

Response::Response() : _responseCode(200) { }

Response::~Response() { }

void Response::addHeader(const std::string& key, const std::string& value)
{
    this->_headers.insert(std::make_pair(key, value));
}
