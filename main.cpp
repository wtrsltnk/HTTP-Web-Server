#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <thread>
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httputility.h"
#include <system.io.directoryinfo.h>
#include <system.io.fileinfo.h>
#include <system.io.path.h>

using namespace std;

class StringRequestHandler : public web::RequestHandler
{
    std::function<void (const std::string&)> _logging;
    std::string _message;
public:
    StringRequestHandler(const std::string& message);
    virtual ~StringRequestHandler();

    void SetLogging(std::function<void (const std::string&)> logging);

    virtual int ConstructResponse(const web::Request &request, web::Response &response);

};

StringRequestHandler::StringRequestHandler(const std::string &message)
    : _message(message), _logging([] (const std::string& message) { std::cout << message << std::endl; })
{ }

StringRequestHandler::~StringRequestHandler() { }

void StringRequestHandler::SetLogging(std::function<void(const std::string&)> logging)
{
    this->_logging = logging;
}

int StringRequestHandler::ConstructResponse(const web::Request &request, web::Response &response)
{
    response._response += this->_message;

    return 200;
}

class FileSystemRequestHandler : public web::RequestHandler
{
    std::function<void (const std::string&)> _logging;
    System::IO::DirectoryInfo _root;
public:
    FileSystemRequestHandler(const std::string& root);
    virtual ~FileSystemRequestHandler();

    void SetLogging(std::function<void (const std::string&)> logging);

    virtual int ConstructResponse(const web::Request &request, web::Response &response);
};

FileSystemRequestHandler::FileSystemRequestHandler(const std::string& root)
    : _root(System::IO::DirectoryInfo(root)), _logging([] (const std::string& message) { std::cout << message << std::endl; })
{ }

FileSystemRequestHandler::~FileSystemRequestHandler() { }

void FileSystemRequestHandler::SetLogging(std::function<void(const std::string&)> logging)
{
    this->_logging = logging;
}

int FileSystemRequestHandler::ConstructResponse(const web::Request &request, web::Response& response)
{
    auto uri = request._uri;
    if (uri[0] != '/')
    {
        return 500;
    }

    uri = uri.substr(1);

    uri = url_decode(uri);

    auto fullPath = System::IO::Path::Combine(this->_root.FullName(), uri);

    _logging(fullPath);

    for (auto header : request._headers)
    {
        std::stringstream sss;
        sss << "[header] " << header.first << ": " << header.second;
        _logging(sss.str());
    }

    std::stringstream ss;

    if (System::IO::DirectoryInfo(fullPath).Exists())
    {
        ss << "<html><body>";

        System::IO::DirectoryInfo directory(fullPath);

        ss << "<h1>" << request._uri << "</h1>";
        ss << "<h2>Directories:</h2>";
        ss << "<ul>";
        if (request._uri != "/")
        {
            auto relativeParentPath = directory.Parent().FullName().substr(this->_root.FullName().size());
            if (relativeParentPath == "") relativeParentPath = "/";
            ss << "<li><a href=\"" << relativeParentPath << "\">..</a></li>";
        }
        for (auto dir : directory.GetDirectories())
        {
            System::IO::DirectoryInfo dirInfo(dir);
            auto relativePath = dirInfo.FullName().substr(this->_root.FullName().size());
            ss << "<li><a href=\"" << relativePath << "\">" << dirInfo.Name() << "</a></li>";
        }
        ss << "</ul>";

        ss << "<h2>Files:</h2>";
        ss << "<ul>";
        for (auto file : directory.GetFiles())
        {
            System::IO::FileInfo fileInfo(file);
            auto relativePath = fileInfo.FullName().substr(this->_root.FullName().size());
            ss << "<li><a href=\"" << relativePath << "\">" << fileInfo.Name() << "</a></li>";
        }
        ss << "</ul>";

        ss << "</body></html>";

        response.addHeader("Content-Type", "text/html");
    }
    else if (System::IO::FileInfo(fullPath).Exists())
    {
        ifstream file;
        auto extpos = fullPath.find_last_of('.');
        auto ext = extpos != std::string::npos ? fullPath.substr(extpos) : "";

        if (ext == ".json")
        {
            file = ifstream(fullPath);
            response.addHeader("Content-Type", "application/json");
        }
        else if (ext == ".jpg")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "image/jpeg");
        }
        else if (ext == ".mp4")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "videos/mp4");
        }
        else if (ext == ".xml")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/xml");
        }
        else if (ext == ".js")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/javascript");
        }
        else if (ext == ".css")
        {
            file = ifstream(fullPath, ios::binary);
            response.addHeader("Content-Type", "application/stylesheet");
        }
        else
        {
            file = ifstream(fullPath);
            response.addHeader("Content-Type", "text/html");
        }

        copy(istreambuf_iterator<char>(file),
             istreambuf_iterator<char>(),
             ostreambuf_iterator<char>(ss));
    }
    else
    {
        ss << "<html><body>";

        ss << request._uri << " does not exist";

        ss << "</body></html>";

        response.addHeader("Content-Type", "text/html");
    }

    response._response += ss.str();

    if (request._headers.find("if-range") != request._headers.end())
    {
        _logging("they want ranges\n");
    }
    if (request._headers.find("Range") != request._headers.end())
    {
        auto range = request._headers.at("Range");
        std::regex rangeRegex("bytes=([0-9]+)-([0-9]*)");
        std::smatch str_match_result;
        if (std::regex_match(range, str_match_result, rangeRegex))
        {
            auto start = std::stoi(str_match_result[1]);
            auto end = str_match_result[2] != "" ? std::stoi(str_match_result[2]) : start + 500;

            response._response = response._response.substr(start, end - start);
            response._responseCode = 206;
        }
    }

    return 200;
}

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <GL/gl.h>


// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets;        // Index to lines offset
    bool                ScrollToBottom;

    void    Clear()     { Buf.clear(); LineOffsets.clear(); }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size);
        ScrollToBottom = true;
    }

    void    Draw(const char* title, bool* p_open = NULL)
    {
        ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
        ImGui::BeginChild(title);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (copy) ImGui::LogToClipboard();

        if (Filter.IsActive())
        {
            const char* buf_begin = Buf.begin();
            const char* line = buf_begin;
            for (int line_no = 0; line != NULL; line_no++)
            {
                const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
                if (Filter.PassFilter(line, line_end))
                    ImGui::TextUnformatted(line, line_end);
                line = line_end && line_end[1] ? line_end + 1 : NULL;
            }
        }
        else
        {
            ImGui::TextUnformatted(Buf.begin());
        }

        if (ScrollToBottom)
            ImGui::SetScrollHere(1.0f);
        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::EndChild();
    }
};

static web::HttpServer server;
static ExampleAppLog log;

static void startServer(System::IO::FileInfo exe)
{
    server.WaitForRequests([exe] (const web::Request request, web::Response & response) -> int {
        std::stringstream ss;
        ss << "Request recieved from " << request.ipAddress()
                  << " for " << request._uri << std::endl;
        log.AddLog(ss.str().c_str());

        FileSystemRequestHandler handler(exe.Directory().FullName());
        handler.SetLogging([] (const std::string& message) {
            log.AddLog(message.c_str());
            log.AddLog("\n");
        });
        return handler.ConstructResponse(request, response);
    });
}

//int main(int argc,char*argv[])
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    std::string cmdLine(lpCmdLine);
    std::string exePath = cmdLine.substr(0, cmdLine.find(' '));

    server.SetLogging([] (const std::string& message) {
        log.AddLog(message.c_str());
        log.AddLog("\n");
    });

    System::IO::FileInfo exe(exePath);

    server.Init();

    while (!server.Start())
    {
        server.SetPortA(server.Port() + 1);
        if (server.Port() > 9999)
        {
            std::cerr << "No available port to listen on" << std::endl;
            return 1;
        }
    }

    std::stringstream title;
    title << "Server listening @ localhost:" << server.Port();

    bool done = false;

    if (!ImGui_ImplWin32GL2_Init(title.str().c_str(), 400, 500))
    {
        std::cerr << "ImgUi Init() failed";
        return 0;
    }

    std::thread t(startServer, exe);
    t.detach();

    bool show_another_window = false;
    bool show_log_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int windowWidth, windowHeight;

    while (!done)
    {
        if (!ImGui_ImplWin32GL2_HandleEvents(done))
        {
            ImGui_ImplWin32GL2_NewFrame(windowWidth, windowHeight);

            // 1. Show a simple window.
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
            ImGui::Begin("Test", &show_another_window, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            {
                ImGui::SetWindowPos(ImVec2(0, 0));
                ImGui::SetWindowSize(ImVec2(windowWidth, windowHeight));

                if (ImGui::Button("Open URL"))
                {
                    ShellExecute(0, 0, server.LocalUrl().c_str(), 0, 0 , SW_SHOW);
                }
                ImGui::SameLine();
                if (ImGui::Button("Open Webroot"))
                {
                    ShellExecute(0, 0, exe.Directory().FullName().c_str(), 0, 0 , SW_SHOW);
                }
                ImGui::SameLine();
                if (ImGui::Button("Quit"))
                {
                    server.Stop();
                    done = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Help"))
                {
                    ShellExecute(0, 0, (server.LocalUrl() + "help").c_str(), 0, 0 , SW_SHOW);
                }
                ImGui::Text("Webroot @ ");
                ImGui::SameLine();
                ImGui::Text(exe.Directory().FullName().c_str());

                ImGui::Text("Server listening @ ");
                ImGui::SameLine();
                ImGui::Text(server.LocalUrl().c_str());

                log.Draw("Example: Log", &show_log_window);

                ImGui::End();
            }

//            ImGui::ShowTestWindow();

            glViewport(0, 0, windowWidth, windowHeight);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
            ImGui_ImplWin32GL2_EndFrame();
        }
    }

    ImGui_ImplWin32GL2_Shutdown();

    return 0;
}

//int main(int argc,char*argv[])
//{
//    auto server = web::HttpServer(8080);

//    server.SetLogging([] (const std::string& message) {
//        std::cout << message << std::endl;
//    });

//    System::IO::FileInfo exe(argv[0]);

//    server.Init();

//    while (!server.Start())
//    {
//        server.SetPortA(server.Port() + 1);
//        if (server.Port() > 9999)
//        {
//            std::cerr << "No available port to listen on" << std::endl;
//            return 1;
//        }
//    }

//    ShellExecute(0, 0, server.LocalUrl().c_str(), 0, 0 , SW_SHOW);

//    server.WaitForRequests([exe] (const web::Request request, web::Response & response) -> int {
//        std::cout << "Request recieved from " << request.ipAddress()
//                  << " for " << request._uri << std::endl;

//        FileSystemRequestHandler handler(exe.Directory().FullName());
//        return handler.ConstructResponse(request, response);
//    });

//    return 0;
//}
