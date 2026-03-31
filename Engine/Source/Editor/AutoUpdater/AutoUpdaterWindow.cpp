#if EDITOR

#include "AutoUpdaterWindow.h"
#include "AutoUpdater.h"
#include "Constants.h"

#include "imgui.h"

static AutoUpdaterWindow sAutoUpdaterWindow;

AutoUpdaterWindow* GetAutoUpdaterWindow()
{
    return &sAutoUpdaterWindow;
}

const char* AutoUpdaterWindow::FormatBytes(size_t bytes, char* buffer, size_t bufferSize)
{
    if (bytes >= 1024 * 1024 * 1024)
    {
        snprintf(buffer, bufferSize, "%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
    else if (bytes >= 1024 * 1024)
    {
        snprintf(buffer, bufferSize, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
    }
    else if (bytes >= 1024)
    {
        snprintf(buffer, bufferSize, "%.2f KB", (double)bytes / 1024.0);
    }
    else
    {
        snprintf(buffer, bufferSize, "%zu bytes", bytes);
    }
    return buffer;
}

void AutoUpdaterWindow::Draw()
{
    AutoUpdater* updater = AutoUpdater::Get();
    if (!updater || !updater->ShouldShowWindow())
    {
        return;
    }

    UpdateCheckState state = updater->GetState();

    // Determine window size based on state
    ImVec2 windowSize(500.0f, 400.0f);
    if (state == UpdateCheckState::Downloading || state == UpdateCheckState::DownloadComplete)
    {
        windowSize.y = 200.0f;
    }
    else if (state == UpdateCheckState::UpToDate || state == UpdateCheckState::Error)
    {
        windowSize.y = 150.0f;
    }

    // Center the window
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 windowPos(
        (io.DisplaySize.x - windowSize.x) * 0.5f,
        (io.DisplaySize.y - windowSize.y) * 0.5f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse;

    const char* title = "Update";
    switch (state)
    {
    case UpdateCheckState::UpdateAvailable:
        title = "Update Available";
        break;
    case UpdateCheckState::Downloading:
        title = "Downloading Update";
        break;
    case UpdateCheckState::DownloadComplete:
        title = "Ready to Install";
        break;
    case UpdateCheckState::UpToDate:
        title = "Up to Date";
        break;
    case UpdateCheckState::Error:
        title = "Update Error";
        break;
    default:
        break;
    }

    bool open = true;
    if (ImGui::Begin(title, &open, flags))
    {
        switch (state)
        {
        case UpdateCheckState::UpdateAvailable:
            DrawUpdateAvailable();
            break;
        case UpdateCheckState::Downloading:
            DrawDownloading();
            break;
        case UpdateCheckState::DownloadComplete:
            DrawDownloadComplete();
            break;
        case UpdateCheckState::UpToDate:
            DrawUpToDate();
            break;
        case UpdateCheckState::Error:
            DrawError();
            break;
        default:
            break;
        }
    }
    ImGui::End();

    if (!open)
    {
        updater->DismissUpdate();
    }
}

void AutoUpdaterWindow::DrawUpdateAvailable()
{
    AutoUpdater* updater = AutoUpdater::Get();
    const ReleaseInfo& release = updater->GetLatestRelease();

    // Version info
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "A new version is available!");
    ImGui::Spacing();

    ImGui::Text("Current version: %s", OCTAVE_VERSION_STRING);
    ImGui::Text("New version: %s", release.GetVersion().c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Changelog
    ImGui::Text("What's New:");
    ImGui::Spacing();

    // Scrollable changelog area
    float changelogHeight = ImGui::GetContentRegionAvail().y - 50.0f;
    if (changelogHeight < 100.0f) changelogHeight = 100.0f;

    ImGui::BeginChild("Changelog", ImVec2(0, changelogHeight), true);
    if (!release.mBody.empty())
    {
        ImGui::TextWrapped("%s", release.mBody.c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No changelog available.");
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // Buttons
    float buttonWidth = 120.0f;
    float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
    if (startX < 0) startX = 0;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

    if (ImGui::Button("Download & Install", ImVec2(buttonWidth, 0)))
    {
        updater->StartDownload();
    }

    ImGui::SameLine();

    if (ImGui::Button("Skip Version", ImVec2(buttonWidth, 0)))
    {
        updater->SkipThisVersion();
    }

    ImGui::SameLine();

    if (ImGui::Button("Remind Later", ImVec2(buttonWidth, 0)))
    {
        updater->DismissUpdate();
    }
}

void AutoUpdaterWindow::DrawDownloading()
{
    AutoUpdater* updater = AutoUpdater::Get();
    const ReleaseInfo& release = updater->GetLatestRelease();

    ImGui::Text("Downloading %s...", release.GetVersion().c_str());
    ImGui::Spacing();

    // Progress bar
    float progress = updater->GetDownloadProgress();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));

    // Progress text
    char downloadedStr[32];
    char totalStr[32];
    FormatBytes(updater->GetDownloadedBytes(), downloadedStr, sizeof(downloadedStr));
    FormatBytes(updater->GetTotalBytes(), totalStr, sizeof(totalStr));

    ImGui::Text("%s / %s (%.1f%%)", downloadedStr, totalStr, progress * 100.0f);

    ImGui::Spacing();

    // Cancel button
    float buttonWidth = 100.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f + ImGui::GetCursorPosX());

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        updater->Cancel();
        updater->DismissUpdate();
    }
}

void AutoUpdaterWindow::DrawDownloadComplete()
{
    AutoUpdater* updater = AutoUpdater::Get();
    const ReleaseInfo& release = updater->GetLatestRelease();

    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Download complete!");
    ImGui::Spacing();

    ImGui::TextWrapped("Version %s has been downloaded. Click 'Install Now' to close the editor and run the installer.",
        release.GetVersion().c_str());

    ImGui::Spacing();

    // Buttons
    float buttonWidth = 120.0f;
    float totalWidth = buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
    if (startX < 0) startX = 0;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

    if (ImGui::Button("Install Now", ImVec2(buttonWidth, 0)))
    {
        updater->LaunchInstaller();
    }

    ImGui::SameLine();

    if (ImGui::Button("Install Later", ImVec2(buttonWidth, 0)))
    {
        updater->DismissUpdate();
    }
}

void AutoUpdaterWindow::DrawUpToDate()
{
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "You're up to date!");
    ImGui::Spacing();

    ImGui::Text("Current version: %s", OCTAVE_VERSION_STRING);

    ImGui::Spacing();

    float buttonWidth = 80.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f + ImGui::GetCursorPosX());

    if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
    {
        AutoUpdater::Get()->DismissUpdate();
    }
}

void AutoUpdaterWindow::DrawError()
{
    AutoUpdater* updater = AutoUpdater::Get();

    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Update check failed");
    ImGui::Spacing();

    const std::string& error = updater->GetErrorMessage();
    if (!error.empty())
    {
        ImGui::TextWrapped("%s", error.c_str());
    }
    else
    {
        ImGui::Text("An unknown error occurred.");
    }

    ImGui::Spacing();

    float buttonWidth = 80.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f + ImGui::GetCursorPosX());

    if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
    {
        updater->DismissUpdate();
    }
}

#endif // EDITOR
