#if EDITOR

#include "TerminalPanel.h"

#include "Preferences/External/CliModule.h"
#include "Preferences/PreferencesManager.h"

#include "Log.h"

#include "imgui.h"

#include <cstring>
#include <string>

static TerminalPanel sTerminalPanel;

TerminalPanel* GetTerminalPanel()
{
    return &sTerminalPanel;
}

namespace
{
    CliModule* FindCliModule()
    {
        PreferencesManager* mgr = PreferencesManager::Get();
        if (mgr == nullptr)
        {
            return nullptr;
        }
        return static_cast<CliModule*>(mgr->FindModule("External/CLI"));
    }

    ImVec4 ColorForKind(TerminalEntryKind kind)
    {
        switch (kind)
        {
            case TerminalEntryKind::Stdout:   return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            case TerminalEntryKind::Stderr:   return ImVec4(1.00f, 0.55f, 0.55f, 1.0f);
            case TerminalEntryKind::System:   return ImVec4(0.50f, 0.80f, 1.00f, 1.0f);
            case TerminalEntryKind::UserEcho: return ImVec4(0.70f, 1.00f, 0.70f, 1.0f);
            default:                          return ImVec4(1, 1, 1, 1);
        }
    }

    ImVec4 ColorForState(TerminalSessionState state)
    {
        switch (state)
        {
            case TerminalSessionState::Stopped:  return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
            case TerminalSessionState::Starting: return ImVec4(1.00f, 0.85f, 0.40f, 1.0f);
            case TerminalSessionState::Running:  return ImVec4(0.40f, 0.90f, 0.40f, 1.0f);
            case TerminalSessionState::Stopping: return ImVec4(1.00f, 0.65f, 0.30f, 1.0f);
            case TerminalSessionState::Error:    return ImVec4(1.00f, 0.40f, 0.40f, 1.0f);
            default:                             return ImVec4(1, 1, 1, 1);
        }
    }
}

void TerminalPanel::Tick()
{
    // Detect open/close transitions to drive auto-launch and auto-close.
    bool opened = (mVisible && !mPrevVisible);
    bool closed = (!mVisible && mPrevVisible);
    mPrevVisible = mVisible;

    if (opened)
    {
        mLaunchedThisOpen = false;
        mFocusInputNextFrame = true;

        CliModule* cli = FindCliModule();
        if (cli != nullptr && cli->mEnabled && cli->mLaunchOnPanelOpen)
        {
            TerminalSessionState s = mSession.GetState();
            if (s == TerminalSessionState::Stopped)
            {
                mSession.Start();
                mLaunchedThisOpen = true;
            }
        }
    }

    if (closed)
    {
        CliModule* cli = FindCliModule();
        if (cli != nullptr && cli->mCloseProcessOnPanelClose)
        {
            if (mSession.GetState() == TerminalSessionState::Running)
            {
                mSession.Stop();
            }
        }
    }

    mSession.Tick();
}

void TerminalPanel::Shutdown()
{
    // Persist the last command if one was set.
    CliModule* cli = FindCliModule();
    if (cli != nullptr)
    {
        cli->mLastCommand = mInputBuffer;
        cli->SetDirty(true);
    }

    mSession.ShutdownSync();
}

void TerminalPanel::DrawContent()
{
    DrawToolbar();
    ImGui::Separator();
    DrawOutput();
    ImGui::Separator();
    DrawInput();
}

void TerminalPanel::DrawToolbar()
{
    TerminalSessionState state = mSession.GetState();
    bool canStart = (state == TerminalSessionState::Stopped || state == TerminalSessionState::Error);
    bool canStop  = (state == TerminalSessionState::Running);

    if (!canStart) ImGui::BeginDisabled();
    if (ImGui::Button("Start"))
    {
        mSession.Start();
        mFocusInputNextFrame = true;
    }
    if (!canStart) ImGui::EndDisabled();

    ImGui::SameLine();
    if (!canStop) ImGui::BeginDisabled();
    if (ImGui::Button("Stop"))
    {
        mSession.Stop();
    }
    if (!canStop) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Restart"))
    {
        mSession.Restart();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        mSession.Clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &mAutoScroll);

    ImGui::SameLine();
    if (ImGui::Button("Preferences..."))
    {
        // Best-effort: just log a hint. The full Preferences window opens via Edit menu.
        LogDebug("[CLI] Open Edit > Preferences > External > CLI to configure the terminal.");
    }

    // State pill on the right.
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::TextColored(ColorForState(state), "[ %s ]", TerminalSessionStateName(state));

    const std::string& label = mSession.GetExecutableLabel();
    if (!label.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", label.c_str());
    }

    if (state == TerminalSessionState::Error && !mSession.GetLastError().empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "(%s)", mSession.GetLastError().c_str());
    }
}

void TerminalPanel::DrawOutput()
{
    // Reserve space for the input box (~one line height + spacing).
    float footerHeight = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##CliTerminalOutput", ImVec2(0, -footerHeight), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

    const auto& entries = mSession.GetBuffer().GetEntries();
    for (const TerminalOutputEntry& entry : entries)
    {
        ImVec4 color = ColorForKind(entry.mKind);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        // Split chunk on '\n' so each line wraps independently and selection works.
        const std::string& s = entry.mText;
        size_t start = 0;
        while (start <= s.size())
        {
            size_t nl = s.find('\n', start);
            size_t lineEnd = (nl == std::string::npos) ? s.size() : nl;
            // Trim trailing CR for cleaner display.
            size_t actualEnd = lineEnd;
            if (actualEnd > start && s[actualEnd - 1] == '\r')
            {
                --actualEnd;
            }
            if (actualEnd > start)
            {
                ImGui::TextUnformatted(s.data() + start, s.data() + actualEnd);
            }
            else if (nl != std::string::npos)
            {
                // Empty line — keep visual spacing.
                ImGui::TextUnformatted("");
            }
            if (nl == std::string::npos)
            {
                break;
            }
            start = nl + 1;
        }

        ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar();

    if (mScrollToBottom || (mAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f))
    {
        ImGui::SetScrollHereY(1.0f);
    }
    mScrollToBottom = false;

    ImGui::EndChild();
}

void TerminalPanel::DrawInput()
{
    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackHistory  |
        ImGuiInputTextFlags_CallbackCompletion;

    if (mFocusInputNextFrame)
    {
        ImGui::SetKeyboardFocusHere();
        mFocusInputNextFrame = false;
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##CliInput", mInputBuffer, sizeof(mInputBuffer), flags, &TerminalPanel::InputCallback, this))
    {
        OnSubmit();
        mFocusInputNextFrame = true;
    }
}

void TerminalPanel::OnSubmit()
{
    std::string line = mInputBuffer;
    mInputBuffer[0] = '\0';

    if (line.empty())
    {
        return;
    }

    if (mSession.GetState() != TerminalSessionState::Running)
    {
        // Not running — try to start, then send.
        if (!mSession.Start())
        {
            return;
        }
    }

    if (mSession.SendCommand(line))
    {
        mHistory.Add(line);
        mScrollToBottom = true;
    }
}

int TerminalPanel::InputCallback(ImGuiInputTextCallbackData* data)
{
    TerminalPanel* panel = static_cast<TerminalPanel*>(data->UserData);
    if (panel == nullptr)
    {
        return 0;
    }

    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        const std::string* item = nullptr;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            item = panel->mHistory.Prev();
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            item = panel->mHistory.Next();
        }

        data->DeleteChars(0, data->BufTextLen);
        if (item != nullptr)
        {
            data->InsertChars(0, item->c_str());
        }
    }

    return 0;
}

#endif
