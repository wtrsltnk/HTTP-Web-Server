#include "stringrequesthandler.h"
#include <iostream>

StringRequestHandler::StringRequestHandler(const std::string &message)
    : _message(message), _logging([] (const std::string& message) { std::cout << message << std::endl; })
{ }

StringRequestHandler::~StringRequestHandler() { }

void StringRequestHandler::SetLogging(std::function<void(const std::string&)> logging)
{
    this->_logging = logging;
}

int StringRequestHandler::ConstructResponse(const web::Request &request, web::Response &response) const
{
    response._response += this->_message;

    return 200;
}
