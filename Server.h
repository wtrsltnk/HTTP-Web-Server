#ifndef _SERVER_H
#define _SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <fstream>
#include <functional>

#define AppVersion "1.0beta"
#define DEFAULT_PORT "8080"
#define BUFFER_SIZE 1024*5 // 5KB
#define ACC_EXT_NUM 48
#define OS_TYPE_NUM 9

using std::string;

extern string ToString(int data);

class Request
{
public:
    Request(SOCKET socket, sockaddr_in clientInfo, const std::string& allData);

    SOCKET _socket;
    sockaddr_in _clientInfo;
    std::string _method;
    std::string _uri;
    std::map<std::string, std::string> _headers;
    std::string _payload;
};

class Server
{
    std::function<void (const std::string&)> _logging;
public:
    Server(std::function<void (const std::string&)> logging);

	// Main Functions
	void Init(); // Initialize Server
    void Start(std::function<bool (Request)> onConnection); // Start server
	void Stop(); // Close Server
	void Communicate(); // Main Loop.
	void ErrorQuit(); // Quit And Display An Error Message


	// IPs Input/Output
	  void setServerIP(string ip);
	  void setClientIP(string ip);
	string getServerIP();
	string getClientIP();

private:
	// Initialize
	void loadContentType();
	void loadOSTypes();
	void resetError();
	
	// Browser Request Manipulation
	void checkRequest();
	void getCommands(); 
	bool isDirectory();
	bool checkFileExists();
	bool checkFileExtension();
	void checkDataLenght();


	 // Pass Info To Variables
	 int getFileRequest();
	 int getUserAgent();
	 int getBrowserInfo();
	 int getOSInfo();
	 int getAcceptedContent();

	 // Header Creation
	 void createHeader();
	 void resetHeader();
	 string _header;
	 string _headerStatus;
	 string _headerType;
	 string _redirectLocation;
	 string _contentLenght;

	// Communication Functions
	 string getMessage(); // Receive Browser Requests
	    int sendFile();
	   void sendHeader(); // Send Header Info
	   void sendMessage(string msg); // Send Text Files
       int  sendData(const char*data); // Send Other Type Files In Binary
	   void closeConnection(); // Close Connections


	// Initialize
	     int _handleReturn;
	    WORD _socketVersion;
	 WSADATA _wsaData;
	  SOCKET _listeningSocket;
 	  SOCKET _connectionSocket;
	addrinfo _hints,*_result;
	     int _maxConnections;

		 
	// Communication Content
		 int _sendData;
		 int _bytesReceived;
		 int _bytesSend;


	//  Server Info
	string _serverIP;
	string _clientIP;
	string _port;
	string _acceptedExtension[ACC_EXT_NUM];
	string _contentType[ACC_EXT_NUM];
	string _OSType[OS_TYPE_NUM];

	// Browser Request Data
	string _browserData;
	string _fileName;
	string _filePath;
	string _fileExt;
	string _fileType;
	   int _fileSize;
	string _userAgentData;
	string _browserInfo;
	string _OSInfo;

};

#endif

