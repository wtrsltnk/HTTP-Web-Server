// ImGui Win32 binding with OpenGL (legacy, fixed pipeline)
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// This code is mostly provided as a reference to learn how ImGui integration works, because it is shorter to read.
// If your code is using GL3+ context or any semi modern OpenGL calls, using this is likely to make everything more
// complicated, will require your code to reset every single OpenGL attributes to their initial state, and might
// confuse your GPU driver. 
// The GL2 code is unable to reset attributes or even call e.g. "glUseProgram(0)" because they don't exist in that API.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui.h>
#include "imgui_impl_win32gl2.h"

#include <iostream>
#include <windows.h>
#include <GL/gl.h>

// Data
static HINSTANCE    g_hInstance = NULL;
static HWND         g_hWnd = NULL;
static HDC          g_hDc = NULL;
static HGLRC        g_hRc = NULL;

static int          g_iWidth = 100;
static int          g_iHeight = 100;
static int          g_iMouseX = 0;
static int          g_iMouseY = 0;

static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static bool         g_MouseJustPressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;

static const char* ImGui_ImplWin32GL2_GetClipboardText(void* user_data)
{
    return 0;//glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplWin32GL2_SetClipboardText(void* user_data, const char* text)
{
//    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

bool ImGui_ImplWin32GL2_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void ImGui_ImplWin32GL2_InvalidateDeviceObjects()
{
    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        break;
    }
    case WM_SIZE:
    {
        g_iWidth = LOWORD(lParam);
        g_iHeight = HIWORD(lParam);
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        // Cannot use 'auto' here, that will create a const reference i guess?
        ImGuiIO& io = ImGui::GetIO();
        io.KeysDown[wParam] = (message == WM_KEYDOWN);

        io.KeyCtrl = io.KeysDown[VK_LCONTROL] || io.KeysDown[VK_RCONTROL];
        io.KeyShift = io.KeysDown[VK_LSHIFT] || io.KeysDown[VK_RSHIFT];
        io.KeyAlt = io.KeysDown[VK_LMENU] || io.KeysDown[VK_RMENU];
        io.KeySuper = io.KeysDown[VK_LWIN] || io.KeysDown[VK_RWIN];
        break;
    }
    case WM_CHAR:
    {
        char c = (char)wParam;
        if (c > 0 && c < 0x10000)
        {
            ImGui::GetIO().AddInputCharacter((unsigned short)c);
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        g_MouseJustPressed[0] = true;
        g_MousePressed[0] = true;
        break;
    }
    case WM_MBUTTONDOWN:
    {
        g_MouseJustPressed[2] = true;
        g_MousePressed[2] = true;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        g_MouseJustPressed[3] = true;
        g_MousePressed[3] = true;
        break;
    }
    case WM_LBUTTONUP:
    {
        g_MousePressed[0] = false;
        break;
    }
    case WM_MBUTTONUP:
    {
        g_MousePressed[2] = false;
        break;
    }
    case WM_RBUTTONUP:
    {
        g_MousePressed[3] = false;
        break;
    }
    case WM_MOUSEMOVE:
    {
        g_iMouseX = LOWORD(lParam);
        g_iMouseY = HIWORD(lParam);

        break;
    }
    case WM_MOUSEWHEEL:
    {
        g_MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / 100;
        break;
    }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void ImGui_ImplWin32GL2_RenderDrawLists(ImDrawData* draw_data);

bool ImGui_ImplWin32GL2_Init(const char *title, int w, int h)
{
    WNDCLASS wc;
    if (!GetClassInfo(g_hInstance, "imguiwindow", &wc))
    {
        ZeroMemory(&wc, sizeof wc);
        wc.hInstance     = g_hInstance;
        wc.lpszClassName = "imguiwindow";
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

        if (FALSE == RegisterClass(&wc))
        {
            std::cerr << "Failed to register window class\n";
            return false;
        }
    }

    g_hWnd = CreateWindowEx(
                WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
                "imguiwindow",
                title,
                WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                w == 0 ? CW_USEDEFAULT : w,
                h == 0 ? CW_USEDEFAULT : h,
                0,
                0,
                g_hInstance,
                NULL);

    if (g_hWnd == NULL)
    {
        std::cerr << "Failed to create window\n";
        return false;
    }

    g_hDc = GetWindowDC(g_hWnd);

    if (g_hDc == NULL)
    {
        DestroyWindow(g_hWnd);

        return false;
    }

    PIXELFORMATDESCRIPTOR pfd;

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(g_hDc, &pfd);

    if (format == 0)
    {
        ReleaseDC(g_hWnd, g_hDc);
        DestroyWindow(g_hWnd);

        return false;
    }

    if (SetPixelFormat(g_hDc, format, &pfd) == FALSE)
    {
        ReleaseDC(g_hWnd, g_hDc);
        DestroyWindow(g_hWnd);

        return false;
    }

    g_hRc = wglCreateContext(g_hDc);

    if (g_hRc == NULL)
    {
        ReleaseDC(g_hWnd, g_hDc);
        DestroyWindow(g_hWnd);

        return false;
    }

    ShowWindow(g_hWnd, SW_SHOW);

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'a';
    io.KeyMap[ImGuiKey_C] = 'c';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    io.RenderDrawListsFn = ImGui_ImplWin32GL2_RenderDrawLists;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplWin32GL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplWin32GL2_GetClipboardText;
    io.ClipboardUserData = g_hWnd;
    io.ImeWindowHandle = g_hWnd;

    if (io.Fonts->AddFontFromFileTTF("c:\\windows\\fonts\\verdana.ttf", 16.0f) == nullptr)
    {
        return false;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.2f, 0.4f, 1.0f);

    return true;
}

bool ImGui_ImplWin32GL2_HandleEvents(bool &done)
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
        {
            done = true;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        return true;
    }

    return false;
}

void ImGui_ImplWin32GL2_Shutdown()
{
    ImGui_ImplWin32GL2_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplWin32GL2_NewFrame(int &width, int &height)
{
    width = g_iWidth;
    height = g_iHeight;

    wglMakeCurrent(g_hDc, g_hRc);

    if (!g_FontTexture)
        ImGui_ImplWin32GL2_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)g_iWidth, (float)g_iHeight);

    // Setup time step
    double current_time =  (GetTickCount() / 1000.0);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    io.MousePos = ImVec2((float)g_iMouseX, (float)g_iMouseY);

    for (int i = 0; i < 3; i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || g_MousePressed[i];
        g_MouseJustPressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
void ImGui_ImplWin32GL2_RenderDrawLists(ImDrawData* draw_data)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
// If text or lines are blurry when integrating ImGui in your engine: in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPolygonMode(GL_FRONT, last_polygon_mode[0]); glPolygonMode(GL_BACK, last_polygon_mode[1]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

void ImGui_ImplWin32GL2_EndFrame()
{
    SwapBuffers(g_hDc);
}
