#pragma once

#include <cstddef> // for size_t

#if EDITOR

/**
 * @brief ImGui modal window for displaying update notifications.
 */
class AutoUpdaterWindow
{
public:
    /**
     * @brief Draw the window. Call each frame from EditorImguiDraw().
     */
    void Draw();

private:
    void DrawUpdateAvailable();
    void DrawDownloading();
    void DrawDownloadComplete();
    void DrawUpToDate();
    void DrawError();

    // Helper to format byte sizes
    static const char* FormatBytes(size_t bytes, char* buffer, size_t bufferSize);
};

/**
 * @brief Get the singleton AutoUpdaterWindow instance.
 */
AutoUpdaterWindow* GetAutoUpdaterWindow();

#endif // EDITOR
