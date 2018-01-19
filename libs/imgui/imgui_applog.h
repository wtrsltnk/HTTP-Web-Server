#ifndef IMGUI_APPLOG_H
#define IMGUI_APPLOG_H

#include <imgui.h>

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

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
    }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
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

    void Draw(const char* title)
    {
        ImGui::BeginChild(title);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            if (copy) ImGui::LogToClipboard();

            if (Filter.IsActive())
            {
                const char* buf_begin = Buf.begin();
                const char* line = buf_begin;
                for (int line_no = 0; line != NULL; line_no++)
                {
                    const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
                    if (Filter.PassFilter(line, line_end))
                    {
                        ImGui::TextUnformatted(line, line_end);
                    }
                    line = line_end && line_end[1] ? line_end + 1 : NULL;
                }
            }
            else
            {
                ImGui::TextUnformatted(Buf.begin());
            }

            if (ScrollToBottom)
            {
                ImGui::SetScrollHere(1.0f);
            }
            ScrollToBottom = false;
            ImGui::EndChild();
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();
    }
};

#endif // IMGUI_APPLOG_H
