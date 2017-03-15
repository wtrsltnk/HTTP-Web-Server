#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Server.h"

using namespace std;

// Global Functions
string  ToString(int data)
{
    std::stringstream buffer;
    buffer << data;
    return buffer.str();
}

// Constructor
Server::Server(std::function<void(const std::string&)> logging)
    : _logging(logging)
{
    // Socket Settings
    ZeroMemory(&_hints,sizeof(_hints));
    _hints.ai_family = AF_INET;
    _hints.ai_flags = AI_PASSIVE;
    _hints.ai_socktype = SOCK_STREAM;
    _hints.ai_protocol = IPPROTO_TCP;

    // Initialize Data
    _maxConnections = 50;
    _listeningSocket  = INVALID_SOCKET;
    _connectionSocket = INVALID_SOCKET;
    _socketVersion = MAKEWORD(2,2);
    _serverIP = "localhost";
    _clientIP = "";
    _port = DEFAULT_PORT;

    // Communication Content
    _bytesReceived = 0;
    _bytesSend = 0;

    //Request Content
    _browserData = "";
    _fileName = "";
    _filePath = "";
    _fileExt = "";
    _browserInfo = "";
    _OSInfo = "";
    _fileSize = 0;

     // Header Content
     _headerStatus = "";
     _headerType= "";
     _redirectLocation = "";
     _contentLenght = "";


}

// Main Functions
void Server::Init()
{
    // Start Winsocket
    _handleReturn = WSAStartup(_socketVersion,&_wsaData);
    if(0 != _handleReturn)
    {
        this->_logging("Initialize Failed \nError Code : " + ToString(_handleReturn));
        ErrorQuit();

    }

    // Load Content Type
    loadContentType();
    loadOSTypes();
}

// The function we want to execute on the new thread.
void task1(std::function<bool (Request)> onConnection, Request request)
{
    std::cout << "===\n";
    if (onConnection(request)) shutdown(request._socket, SD_BOTH);
    std::cout << "===\n";
}

void Server::Start(std::function<bool (Request)> onConnection)
{
    char tempSTR[INET_ADDRSTRLEN];

    // Resolve Local Address And Port
    _handleReturn = getaddrinfo(NULL,_port.c_str(),&_hints,&_result);
    if(0 != _handleReturn)
    {
        this->_logging("Resolving Address And Port Failed \nError Code: " + ToString(_handleReturn));
        ErrorQuit();
    }

    // Create Socket
    _listeningSocket = socket(_hints.ai_family,_hints.ai_socktype,_hints.ai_protocol);
    if( INVALID_SOCKET == _listeningSocket )
    {
        this->_logging("Could't Create Socket");
        ErrorQuit();
    }

    // Bind
    _handleReturn = bind(_listeningSocket,_result->ai_addr,(int)_result->ai_addrlen);
    if(SOCKET_ERROR == _handleReturn)
    {
        this->_logging("Bind Socket Failed");
        ErrorQuit();
    }

    while(true)
    {
        // Listen
        _handleReturn = listen(_listeningSocket,_maxConnections);
        if(SOCKET_ERROR == _handleReturn)
        {
            this->_logging("Listening On Port " + _port + " Failed");
            ErrorQuit();
        }
        else
        {
            cout << "-Server Is Up And Running.\n--Listening On Port " << _port << "..." << endl;
        }

        sockaddr_in clientInfo;
        int clientInfoSize = sizeof(clientInfo);

        _connectionSocket = accept(_listeningSocket,(sockaddr*)&clientInfo,&clientInfoSize);
        if(INVALID_SOCKET == _connectionSocket)
        {
            this->_logging("Accepting Connection Failed");
        }
        else
        {
            setClientIP(inet_ntoa(clientInfo.sin_addr));

            std::thread t(task1, onConnection, Request(_connectionSocket, clientInfo, getMessage()));
            t.detach();

            // Main Procedure
            Communicate();

            // Close Connection And Reset Some Vars
            closeConnection();
            resetError();
            resetHeader();
        }
    }
}
void Server::Stop()
{
    closesocket(_listeningSocket);
    closesocket(_connectionSocket);
    WSACleanup();
}

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

Request::Request(SOCKET socket, sockaddr_in clientInfo, const std::string& allData)
    : _socket(socket), _clientInfo(clientInfo)
{
    auto pos = allData.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::stringstream ss;
        ss.str(allData.substr(0, pos));
        std::string line;

        // Determine methode, uri and HTTP version
        if (std::getline(ss, line))
        {
            trim(line);
            auto first = line.find_first_of(' ');
            auto last = line.find_last_of(' ');
            if (first != std::string::npos && last != std::string::npos)
            {
                auto version = line.substr(last);
                if (trim(version) != "HTTP/1.1") throw new std::runtime_error("invalid HTTP version");

                auto method = line.substr(0, first);
                this->_method = trim(method);

                auto uri = line.substr(first, last-first);
                this->_uri = trim(uri);
            }
        }

        // Determine headers
        while (std::getline(ss, line))
        {
            auto pos = line.find_first_of(':');
            if (pos != std::string::npos)
            {
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                this->_headers.insert(std::make_pair(trim(key), trim(value)));
            }
        }

        // And finally determine payload(if any)
        this->_payload = allData.substr(pos + 4);
    }
}

void Server::Communicate()
{
    // Check Browser's Request
    checkRequest();

    // Create Header And Send It To Browser
    createHeader();
    sendHeader();

    // Temp Display header

    if(_headerType != "" && _contentLenght != "")
    {
        cout << "- " << _headerType <<
                "- " << _contentLenght;
    }

    // Send Data If there wasn't an error
    sendFile();
}
void Server::ErrorQuit()
{
    Stop();
    cout << "An Error Occurred.Check \"error.txt\" For More Info" << endl;
//	system("pause");
    exit(-1);
}
void Server::loadContentType()
{
    _acceptedExtension[0] = "html";		_contentType[0] = "text/html";
    _acceptedExtension[1] = "htm";		_contentType[1] = "text/html";
    _acceptedExtension[2] = "htx";		_contentType[2] = "text/html";
    _acceptedExtension[3] = "shtml";	_contentType[3] = "text/html";
    _acceptedExtension[4] = "stm";		_contentType[4] = "text/html";
    _acceptedExtension[5] = "css";		_contentType[5] = "text/css";
    _acceptedExtension[6] = "xml";		_contentType[6] = "text/xml";
    _acceptedExtension[7] = "txt";		_contentType[7] = "text/plain";

    _acceptedExtension[8] = "jpeg";		_contentType[8] = "image/jpeg";
    _acceptedExtension[9] = "jpe";		_contentType[9] = "image/jpeg";
    _acceptedExtension[10] = "jpg";		_contentType[10] = "image/jpeg";
    _acceptedExtension[11] = "gif";		_contentType[11] = "image/gif";
    _acceptedExtension[12] = "png";		_contentType[12] = "image/png";
    _acceptedExtension[13] = "ico";		_contentType[13] = "image/x-icon";
    _acceptedExtension[14] = "bmp";		_contentType[14] = "image/x-bmp";
    _acceptedExtension[15] = "tga";		_contentType[15] = "image/targa";

    _acceptedExtension[16] = "gz";		_contentType[16] = "application/x-compressed ";
    _acceptedExtension[17] = "tar";		_contentType[17] = "application/x-compressed ";
    _acceptedExtension[18] = "zip";		_contentType[18] = "application/x-zip-compressed";
    _acceptedExtension[19] = "rar";		_contentType[19] = "application/x-rar-compressed";

    _acceptedExtension[20] = "exe";		_contentType[20] = "application/x-msdownload";
    _acceptedExtension[21] = "dll";		_contentType[21] = "application/x-msdownload";

    _acceptedExtension[22] = "pdf";		_contentType[22] = "application/pdf";
    _acceptedExtension[23] = "swf";		_contentType[23] = "application/x-shockwave-flash";

    _acceptedExtension[24] = "ac3";		_contentType[24] = "audio/ac3";
    _acceptedExtension[25] = "au";		_contentType[25] = "audio/basic";
    _acceptedExtension[26] = "m3u";		_contentType[26] = "audio/x-mpegurl";
    _acceptedExtension[27] = "mid";		_contentType[27] = "audio/mid";
    _acceptedExtension[28] = "midi";	_contentType[28] = "audio/mid";
    _acceptedExtension[29] = "mp3";		_contentType[29] = "audio/mpeg";
    _acceptedExtension[30] = "rmi";		_contentType[30] = "audio/mid";
    _acceptedExtension[31] = "snd";		_contentType[31] = "audio/basic";
    _acceptedExtension[32] = "wav";		_contentType[32] = "audio/wav";
    _acceptedExtension[33] = "wma";		_contentType[33] = "audio/x-ms-wma";
    _acceptedExtension[34] = "wave";	_contentType[34] = "audio/wav";
    _acceptedExtension[35] = "aiff";	_contentType[35] = "audio/aiff";


    _acceptedExtension[36] = "3gp";		_contentType[36] = "video/3gpp";
    _acceptedExtension[37] = "3gp2";	_contentType[37] = "video/3gpp2";
    _acceptedExtension[38] = "asf";		_contentType[38] = "video/x-ms-asf";
    _acceptedExtension[39] = "asx";		_contentType[39] = "video/x-ms-asf";
    _acceptedExtension[40] = "avi";		_contentType[40] = "video/avi";
    _acceptedExtension[41] = "flv";		_contentType[41] = "video/x-flv";
    _acceptedExtension[42] = "mp2";		_contentType[42] = "video/mpeg";
    _acceptedExtension[43] = "mp4";		_contentType[43] = "video/mp4";
    _acceptedExtension[44] = "mpeg";	_contentType[44] = "video/x-ogg";
    _acceptedExtension[45] = "ogg";		_contentType[45] = "audio/x-ms-wma";
    _acceptedExtension[46] = "qt";		_contentType[46] = "video/quicktime";
    _acceptedExtension[47] = "wmv";		_contentType[47] = "video/x-ms-wmv";





}
void Server::loadOSTypes()
{
    _OSType[0] = "Windows NT";
    _OSType[1] = "Linux";
    _OSType[2] = "Intel Mac _OSType X";
    _OSType[3] = "FreeBSD";
    _OSType[4] = "OpenBSD";
    _OSType[5] = "Nintendo Wii";
    _OSType[6] = "PlayStation Portable";
    _OSType[7] = "PLAYSTATION 3";
    _OSType[8] = "iPhone";
}
void Server::resetError()
{
    _sendData = 0;
}

// IPs Input/Output
void Server::setServerIP(string ip)
{
    _serverIP = ip;
}
void Server::setClientIP(string ip)
{
    _clientIP = ip;
}

static std::string __401data = "<h1>401, page not exists</h1>";

// Browser Request
void Server::checkRequest()
{
    // Check Request Commands
    getCommands();
    // If No Errors Occured
    if(_sendData != -1)
    {
        if(checkFileExists())
        {
            if(checkFileExtension())
            {
                checkDataLenght();
            }
        }
        else
        {
            _fileSize = __401data.size();
            _contentLenght = "Content-Length: " + ToString(_fileSize) +"\r\n";
            resetError();
        }
    }
}

void Server::getCommands()
{
    _sendData = getFileRequest();
    if(_sendData != -1)
    {
        getUserAgent();
        getAcceptedContent();
    }
}

int Server::getFileRequest()
{
    // tempCode
    //cout << _browserData << endl;

    int filePathStart,filePathEnd;
    // Path And File Is Between GET and HTTP
    filePathStart = _browserData.find("GET ") +4;
    filePathEnd = _browserData.find(" HTTP") -1;
    if(string::npos == filePathStart || string::npos == filePathEnd)
    {
        this->_logging("HTTP/1.1 400 Bad Request Of " + _clientIP + "\n" );
        _headerStatus = "HTTP/1.1 400 Bad Request\r\n";
        return(-1);
    }

    // I'm Checking Slashes so to find the last slash to find the path range
    int slashPosition=0;
    for(int i=filePathStart;i<=filePathEnd;i++)
    {
        if(_browserData[i] == '/')
            slashPosition=i;
        else if(_browserData[i] == '.')
            break;
    }

    // Set File Path And Name
    _filePath = _browserData.substr(filePathStart+1,(slashPosition - filePathStart));
    _fileName = _browserData.substr(slashPosition+1,(filePathEnd - slashPosition));

        // Check If filename has extension
        bool extFound=false;
        for(int i=0;i<_fileName.size();i++)
        {
            if(_fileName[i] == '.')
            {
                extFound = true;
                break;
            }
        }

        // If Not Set Header Status To Redirection
        if(!extFound)
        {
            _headerStatus = "HTTP/1.1 301 Moved Permanently\r\n";
            _redirectLocation = "Location: index.html"; //+ _filePath + _fileName + "index.html"  + "\r\n";
            return(-1);
        }

    // If Path is Slash Then Filename Is index.html
    if(_fileName == "")
    {
        _fileName = "index.html";
    }

    return 0;

}
int Server::getUserAgent()
{
        string UsAgent = "User-Agent: ";
    int UsAgentStart=0;
    int UsAgentEnd=0;

    UsAgentStart = _browserData.find(UsAgent);
    if(UsAgentStart == string::npos)
    {
        return -1;
    }

    // Search For EndLine -- MAX LENGHT 400
    int searchCR = UsAgentStart + UsAgent.size();
    while(true)
    {
        if(_browserData[searchCR] == '\r')
        {
            UsAgentEnd = searchCR;
            break;
        }
        else if(searchCR > (UsAgentStart + UsAgent.size() +400))
        {
            this->_logging("Could't Find CR in User-Agent For " + _clientIP);
            return -1;
        }
        searchCR++;
    }


    _userAgentData = _browserData.substr(UsAgentStart,UsAgentEnd - UsAgentStart);

    // Search For _OSType
    getOSInfo();

    return 0;
}
int Server::getAcceptedContent()
{
    return 0;
}

// File Request Content
bool Server::checkFileExtension()
{
    string extension="";
    for(int i=0;i<_fileName.size();i++)
    {
        if(_fileName[i] == '.')
        {
            extension = _fileName.substr(i+1,_fileName.size()-1);
            break;
        }
    }

    // If there was an error return false
    if(extension=="")
    {
        this->_logging("Extension Of File Didn't Found");
        return false;
    }

    for(int i=0;i<ACC_EXT_NUM;i++)
    {

        if(_acceptedExtension[i] == extension)
        {
            _fileExt = extension;
            _fileType = _contentType[i];
            _headerType = "Content-Type: " + _contentType[i] + "\r\n";
            return true;
        }
    }

    return false;

}
bool Server::checkFileExists()
{
    ifstream file(_filePath + _fileName);
    if(file.is_open())
    {
        file.close();
        return true;
    }
    else
    {
        _sendData = -1;
        _headerStatus = "HTTP/1.1 404 Not Found\r\n";
        this->_logging("HTTP/1.1 404 File " + _filePath + _fileName + " Not Found For " + _clientIP);

        return false;
    }
}
void Server::checkDataLenght()
{
    std::ifstream file(_filePath + _fileName, ios::binary | ios::ate);
    if(file.is_open())
    {
        _fileSize = file.tellg();
        _contentLenght = "Content-Length: " + ToString(_fileSize) +"\r\n";
        file.seekg(0,ios::beg);
        file.close();
    }
}

// User-Agent Content
int Server::getOSInfo()
{

    int startOS = string::npos;
    int endOS  = string::npos;



    // Check OS And Position
    int OSn=0;
    for(;OSn<OS_TYPE_NUM;OSn++)
    {
        startOS = _userAgentData.find(_OSType[OSn]);

        if(startOS != string::npos) // _OSType FOUND
        {
            break;
        }
        else if((OSn == OS_TYPE_NUM-1) && (startOS == string::npos))
        {
            this->_logging("Couldn't Recognize _OSType Of " + _clientIP );
            _OSInfo = "Unknown _OSType";
            return -1;
        }
    }

    // Find The End Of The _OSType String
    endOS = startOS + _OSType[OSn].size();
    while(true)
    {
        if(_userAgentData[endOS] == ';')
        {
            break;
        }
        else if(endOS == _userAgentData.size()-1 )
        {
            this->_logging("Couldn't Find _OSType End String For " + _clientIP + " Maybe Browser Is Old And Unsupported");
            _OSInfo = "Unknown _OSType";
            return -1;
        }
        endOS++;
    }
    _OSInfo = _userAgentData.substr(startOS,endOS-startOS);

    return 0;
}
int Server::getBrowserInfo()
{
    return 0;
}

// Header Functions
void Server::createHeader()
{
    if(_sendData==-1)
        _header = _headerStatus;
    else
        _header = "HTTP/1.1 200 OK\r\n";

    _header += _headerType;
    _header += _contentLenght;
        if(_redirectLocation != "")
            _header += _redirectLocation;
}
void Server::resetHeader()
{
    _header = "";
    _headerStatus = "";
    _headerType = "";
    _redirectLocation = "";
    _contentLenght = "";
}

// Communication Functions
int Server::sendFile()
{
    if(_sendData==-1)
    {
        sendData(__401data.c_str());
        return 0;
    }

    // Find Type Of Content
    int slashPosition = 0;
    for(;slashPosition<_fileType.size();slashPosition++)
    {
        if(_fileType[slashPosition] == '/')
        {
            break;
        }
    }
    string type = _fileType.substr(0,slashPosition);
    if(type == "text")
    {
        // Send Page
        std::ifstream file(_filePath + _fileName, ios::ate);
        char fileBuffer[BUFFER_SIZE];
        int bytesRead = 0;
        if(file.is_open())
        {
            int size = file.tellg();
            file.seekg(0,ios::beg);

            while(!file.eof())
            {
                file.read(fileBuffer,BUFFER_SIZE);
                fileBuffer[_fileSize] = '\0';
                sendMessage(fileBuffer);
                bytesRead += BUFFER_SIZE;
                cout << "  " << bytesRead << " Bytes Send Of " << size << endl;
            }
            file.close();
        }
    }
    else
    {
        // Open File -- Binary Flag And Ate Flag (goes to the end of file so we can find the size)
        std::ifstream file(_filePath + _fileName, ios::binary|ios::ate);
        char fileBuffer[BUFFER_SIZE];
        int bytesRead =0;
        int bytesSend =0;
        if(file.is_open())
        {
            int size = file.tellg();
            file.seekg(0,ios::beg);

            while(bytesRead<size && bytesSend != -1)//bytesRead<size)
            {
                file.read(fileBuffer,BUFFER_SIZE);
                bytesRead += BUFFER_SIZE;
                cout << "  " << bytesRead << " Bytes Send Of " << size << endl;

                bytesSend = sendData(fileBuffer);
            }
            file.close();
        }
    }
    return 0;
}

void Server::sendHeader()
{
    sendMessage(_header + "\r\n");
}

string Server::getMessage()
{
    char buffer[BUFFER_SIZE+1];
    int bytes;

    bytes = recv(_connectionSocket, buffer, BUFFER_SIZE, 0);
    buffer[bytes] = '\0';
    _browserData = buffer;

    while(BUFFER_SIZE == bytes)
    {
        bytes = recv(_connectionSocket, buffer, BUFFER_SIZE, 0);
        buffer[bytes] = '\0';
        _browserData += buffer;
    }

    return _browserData;
}
void Server::sendMessage(string msg)
{
    _bytesSend += send(_connectionSocket,msg.c_str(),msg.size(),0);
}
int Server::sendData(const char* data)
{
    _bytesSend += send(_connectionSocket,data,BUFFER_SIZE,0);
    return _bytesSend;
}
void Server::closeConnection()
{

    if(shutdown(_connectionSocket,SD_BOTH) == SOCKET_ERROR)
    {

        this->_logging("ShutDown Connection Failed");
        exit(-1);
    }

    _bytesReceived = 0;
    _bytesSend = 0;

    _clientIP = "";
    _browserData = "";
    _fileName = "";
    _filePath = "";
    _fileExt ="";
    _fileType = "";
    _fileSize = 0;
    _userAgentData = "";

    _browserInfo = "";
    _OSInfo = "";

}



