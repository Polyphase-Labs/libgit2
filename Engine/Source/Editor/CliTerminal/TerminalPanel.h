#pragma once

#if EDITOR

#include "CommandHistory.h"
#include "TerminalSession.h"

struct ImGuiInputTextCallbackData;

/**
 * @brief Singleton dockable CLI Terminal panel.
 *
 * Owns one TerminalSession and one CommandHistory. Drawn from the editor
 * dockspace via DrawContent(). Tick() is called every frame from EditorMain
 * to drain output and advance the session state machine. Shutdown() is
 * called from EditorImguiPreShutdown to ensure all child processes and
 * worker threads are torn down before the rest of the editor.
 */
class TerminalPanel
{
public:
    void DrawContent();
    void Tick();
    void Shutdown();

    bool mVisible = false;

private:
    void DrawToolbar();
    void DrawOutput();
    void DrawInput();
    void OnSubmit();

    static int InputCallback(ImGuiInputTextCallbackData* data);

    TerminalSession mSession;
    CommandHistory  mHistory;

    char mInputBuffer[2048] = {};
    bool mAutoScroll = true;
    bool mFocusInputNextFrame = false;
    bool mPrevVisible = false;
    bool mLaunchedThisOpen = false;

    bool mScrollToBottom = false;
};

TerminalPanel* GetTerminalPanel();

#endif
