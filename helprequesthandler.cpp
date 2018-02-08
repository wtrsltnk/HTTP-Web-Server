#include "helprequesthandler.h"
#include "httpresponse.h"
#include <sstream>

HelpRequestHandler::HelpRequestHandler()
{ }

int HelpRequestHandler::ConstructResponse(const web::Request &request, web::Response &response) const
{
    std::stringstream ss;

    ss << "<html><body>";

    ss << "<h1>Help</h1>";

    ss << "</body></html>";

    response.addHeader("Content-Type", "text/html");

    response._response += ss.str();

    return 200;
}
