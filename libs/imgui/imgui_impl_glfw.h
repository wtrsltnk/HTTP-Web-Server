// ImGui GLFW binding with OpenGL (legacy, fixed pipeline)
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_ImplWin32GL2_Init(const char *title, int w = 0, int h = 0);
IMGUI_API void        ImGui_ImplWin32GL2_Shutdown();
IMGUI_API void        ImGui_ImplWin32GL2_NewFrame(int &width, int &height);
IMGUI_API bool        ImGui_ImplWin32GL2_HandleEvents(bool &done);
IMGUI_API void        ImGui_ImplWin32GL2_EndFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplWin32GL2_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplWin32GL2_CreateDeviceObjects();
