
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "filesystemrequesthandler.h"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <system.io.directoryinfo.h>
#include <system.io.fileinfo.h>
#include <system.io.path.h>
#include <imgui.h>
#include <imgui_impl_win32gl2.h>
#include <imgui_applog.h>
#include <GL/gl.h>

static web::HttpServer server;
static ExampleAppLog log;
static FileSystemRequestHandler handler;

static void startServer(System::IO::FileInfo exe)
{
    server.WaitForRequests([exe] (const web::Request request, web::Response & response) -> int {
        log.AddLog("Request recieved from %s for %s", request.ipAddress().c_str(), request._uri.c_str());

        return handler.ConstructResponse(request, response);
    });
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR szFileName[MAX_PATH + 1];
    GetModuleFileName(NULL, szFileName, MAX_PATH + 1);

    std::string exePath(szFileName);

    server.SetLogging([] (const std::string& message) {
        log.AddLog((message + "\n").c_str());
    });

    System::IO::FileInfo exe(exePath);

    handler.SetRoot(exe.Directory().FullName());
    handler.SetLogging([] (const std::string& message) {
        log.AddLog(message.c_str());
        log.AddLog("\n");
    });

    System::IO::FileInfo mimeTypesPath("mimetypes.config");

    if (mimeTypesPath.Exists())
    {
        handler.GetMimeTypes().LoadFromFile(mimeTypesPath.FullName());
    }

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

    if (!ImGui_ImplWin32GL2_Init(title.str().c_str(), 800, 600))
    {
        std::cerr << "ImGui Init() failed";
        return 0;
    }

    std::thread t(startServer, exe);
    t.detach();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int windowWidth = 1, windowHeight = 1;

    std::unique_ptr<bool[]> mimeTypeSelection(new bool[handler.GetMimeTypes().Data().size()]);
    for (int i = 0; i < handler.GetMimeTypes().Data().size(); i++)
    {
        mimeTypeSelection[i] = false;
    }

    while (!done)
    {
        if (!ImGui_ImplWin32GL2_HandleEvents(done))
        {
            ImGui_ImplWin32GL2_NewFrame(windowWidth, windowHeight);

            ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            {
                ImGui::SetWindowPos(ImVec2(0, 0));
                ImGui::SetWindowSize(ImVec2(windowWidth / 2.0f, windowHeight));

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

                log.Draw("Request Log");

                ImGui::End();
            }

            ImGui::Begin("Mime types", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            {
                ImGui::SetWindowPos(ImVec2(windowWidth / 2.0f, 0));
                ImGui::SetWindowSize(ImVec2(windowWidth / 2.0f, windowHeight));

                static char Extension[8] = { 0 };
                ImGui::InputText("Extension", Extension, 8);
                static char Mimetype[64] = { 0 };
                ImGui::InputText("Mimetype", Mimetype, 64);
                auto mimetypes = handler.GetMimeTypes().Data();
                if (ImGui::Button("Add Mimetype"))
                {
                    if (Extension[0] == '.')
                    {
                        handler.GetMimeTypes().Add(Extension, Mimetype);
                    }
                }
                ImGui::Separator();
                if (ImGui::Button("Delete selected"))
                {
                    std::vector<std::string> extensionsToDelete;
                    int i = 0;
                    for (auto pair : mimetypes)
                    {
                        if (mimeTypeSelection[i++])
                        {
                            extensionsToDelete.push_back(pair.first);
                            std::cout << "removeing " << pair.first << std::endl;
                        }
                    }
                    for (auto r : extensionsToDelete)
                    {
                        handler.GetMimeTypes().Remove(r);
                    }

                    mimeTypeSelection = std::unique_ptr<bool[]>(new bool[handler.GetMimeTypes().Data().size()]);
                    for (int i = 0; i < handler.GetMimeTypes().Data().size(); i++)
                    {
                        mimeTypeSelection[i] = false;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear selected"))
                {
                    for (int i = 0; i < handler.GetMimeTypes().Data().size(); i++)
                    {
                        mimeTypeSelection[i] = false;
                    }
                }
                if (ImGui::Button("Save mimetypes"))
                {
                    if (!handler.GetMimeTypes().SaveToFile(mimeTypesPath.FullName()))
                    {
                        log.AddLog("Failed to save mimetypes");
                    }
                    else
                    {
                        log.AddLog("Mimetypes saved");
                    }
                }
                ImGui::Columns(3, "Mime types", false);
                ImGui::SetColumnWidth(-1, 80.0f);
                ImGui::Text("Ext"); ImGui::NextColumn();
                ImGui::SetColumnWidth(-1, 270.0f);
                ImGui::Text("Mimetype"); ImGui::NextColumn();
                ImGui::SetColumnWidth(-1, 100.0f);
                ImGui::Text(""); ImGui::NextColumn();
                ImGui::Separator();
                int i = 0;
                for (auto pair : mimetypes)
                {
                    ImGui::Selectable(pair.first.c_str(), &(mimeTypeSelection[i])); ImGui::NextColumn();
                    ImGui::Selectable(pair.second.c_str(), &(mimeTypeSelection[i])); ImGui::NextColumn();
                    ImGui::NextColumn();
                    i++;
                }

                ImGui::End();
            }

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
