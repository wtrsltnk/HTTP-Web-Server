#include <iostream>
#include <string>
#include "Server.h"

using namespace std;

int main(int argc,char*argv[])
{
    auto server = HttpServer([] (const std::string& message) {
        std::cout << message << std::endl;
    });

    server.Init();

    server.Start([] (Request request) -> Response {
        std::cout << "Request recieved from " << inet_ntoa(request._clientInfo.sin_addr)
                  << " for " << request._uri << std::endl;

        return MessageRequestHandler("<html><body>hi</body></html>")
                .ConstructResponse(request);
    });

	return 0;
}
