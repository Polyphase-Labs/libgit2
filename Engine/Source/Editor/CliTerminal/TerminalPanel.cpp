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
    DrawTuiKeys();
    ImGui::Separator();
    DrawOutput();
    ImGui::Separator();
    DrawInput();
}

void TerminalPanel::DrawTuiKeys()
{
    // Compact row of "send raw key" buttons for navigating interactive TUI
    // prompts (arrow keys for menus, Enter to confirm, Esc/Ctrl+C to abort).
    // Only useful when the session is running and connected to a real TTY,
    // but we render them whenever the session is running so the user can
    // also use them with pipe-mode shells if desired.
    bool running = (mSession.GetState() == TerminalSessionState::Running);
    if (!running) ImGui::BeginDisabled();

    auto sendKey = [this](const char* seq, size_t len) {
        mSession.SendRaw(seq, len);
        mScrollToBottom = true;
    };

    if (ImGui::SmallButton("Up"))    sendKey("\x1b[A", 3);
    ImGui::SameLine();
    if (ImGui::SmallButton("Down"))  sendKey("\x1b[B", 3);
    ImGui::SameLine();
    if (ImGui::SmallButton("Left"))  sendKey("\x1b[D", 3);
    ImGui::SameLine();
    if (ImGui::SmallButton("Right")) sendKey("\x1b[C", 3);
    ImGui::SameLine();
    if (ImGui::SmallButton("Enter")) sendKey("\r", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("Tab"))   sendKey("\t", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("Esc"))   sendKey("\x1b", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("Space")) sendKey(" ", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("^C"))    sendKey("\x03", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("y"))     sendKey("y", 1);
    ImGui::SameLine();
    if (ImGui::SmallButton("n"))     sendKey("n", 1);

    if (!running) ImGui::EndDisabled();
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
    if (ImGui::Button("Copy All"))
    {
        std::string all;
        const auto& entries = mSession.GetBuffer().GetEntries();
        all.reserve(entries.size() * 32);
        for (const TerminalOutputEntry& e : entries)
        {
            all.append(e.mText);
            if (!e.mText.empty() && e.mText.back() != '\n')
            {
                all.push_back('\n');
            }
        }
        ImGui::SetClipboardText(all.c_str());
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

    // One-shot diagnostic: log when entries first becomes non-empty so we
    // can confirm the panel sees the buffer.
    {
        static bool sLoggedNonEmpty = false;
        if (!sLoggedNonEmpty && !entries.empty())
        {
            sLoggedNonEmpty = true;
            LogDebug("[CLI] DrawOutput: first frame with entries, count=%zu buffer=%p",
                     entries.size(), (const void*)&mSession.GetBuffer());
        }
    }

    // Render each entry as one or more Selectable lines so the user can
    // click to highlight a line and right-click to copy. We split each
    // chunk on '\n' so multi-line chunks become multiple selectable rows.
    int globalLineIndex = 0;
    for (const TerminalOutputEntry& entry : entries)
    {
        ImVec4 color = ColorForKind(entry.mKind);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        const std::string& s = entry.mText;
        size_t start = 0;
        while (start <= s.size())
        {
            size_t nl = s.find('\n', start);
            size_t lineEnd = (nl == std::string::npos) ? s.size() : nl;
            size_t actualEnd = lineEnd;
            if (actualEnd > start && s[actualEnd - 1] == '\r')
            {
                --actualEnd;
            }

            // Build a stable temporary std::string for the line so we can
            // pass it to Selectable (which needs a null-terminated label)
            // and to SetClipboardText.
            std::string line;
            if (actualEnd > start)
            {
                line.assign(s.data() + start, actualEnd - start);
            }

            // Use the line index as the ID so identical lines don't collide.
            ImGui::PushID(globalLineIndex++);

            const char* label = line.empty() ? " " : line.c_str();
            bool selected = (mSelectedLineIndex == globalLineIndex - 1);
            if (ImGui::Selectable(label, selected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                mSelectedLineIndex = globalLineIndex - 1;
                mSelectedLineText = line;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    ImGui::SetClipboardText(line.c_str());
                }
            }

            if (ImGui::BeginPopupContextItem("##LineCtx"))
            {
                if (ImGui::MenuItem("Copy line"))
                {
                    ImGui::SetClipboardText(line.c_str());
                }
                if (ImGui::MenuItem("Copy all"))
                {
                    std::string all;
                    all.reserve(entries.size() * 32);
                    for (const TerminalOutputEntry& e : entries)
                    {
                        all.append(e.mText);
                        if (!e.mText.empty() && e.mText.back() != '\n')
                        {
                            all.push_back('\n');
                        }
                    }
                    ImGui::SetClipboardText(all.c_str());
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();

            if (nl == std::string::npos)
            {
                break;
            }
            start = nl + 1;
        }

        ImGui::PopStyleColor();
    }

    // Ctrl+C while the output area is hovered copies the currently selected
    // line, or the whole buffer if nothing is selected.
    if (ImGui::IsWindowHovered() && ImGui::IsKeyPressed(ImGuiKey_C) &&
        (ImGui::GetIO().KeyCtrl))
    {
        if (!mSelectedLineText.empty())
        {
            ImGui::SetClipboardText(mSelectedLineText.c_str());
        }
        else
        {
            std::string all;
            all.reserve(entries.size() * 32);
            for (const TerminalOutputEntry& e : entries)
            {
                all.append(e.mText);
                if (!e.mText.empty() && e.mText.back() != '\n')
                {
                    all.push_back('\n');
                }
            }
            ImGui::SetClipboardText(all.c_str());
        }
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
