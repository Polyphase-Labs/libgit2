#if EDITOR

#include "GitMergeDialog.h"
#include "../GitService.h"
#include "../GitRepository.h"
#include "../GitProcessRunner.h"
#include "imgui.h"
#include <cstring>
#include <thread>

static GitMergeDialog sInstance;

GitMergeDialog* GetGitMergeDialog()
{
    return &sInstance;
}

void GitMergeDialog::Open()
{
    mIsOpen = true;
    mJustOpened = true;
    mSelectedSourceBranch = 0;
    mFastForwardOnly = false;
    mMergeInProgress = false;
    mMergeResult.clear();

    mBranchNames.clear();
    mCurrentBranch.clear();

    GitService* service = GitService::Get();
    if (service && service->IsRepositoryOpen())
    {
        GitRepository* repo = service->GetCurrentRepo();
        mCurrentBranch = repo->GetCurrentBranch();

        std::vector<GitBranchInfo> branches = repo->GetBranches();
        for (const auto& branch : branches)
        {
            // Include all branches except the current one
            if (branch.mName != mCurrentBranch)
            {
                mBranchNames.push_back(branch.mName);
            }
        }
    }
}

void GitMergeDialog::Draw()
{
    if (!mIsOpen) return;

    if (mJustOpened)
    {
        ImGui::OpenPopup("Merge");
        mJustOpened = false;
    }

    ImVec2 center = ImGui::GetIO().DisplaySize;
    center.x *= 0.5f;
    center.y *= 0.5f;
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Merge", &mIsOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        bool hasBranches = !mBranchNames.empty();

        // Current branch display
        ImGui::Text("Current Branch");
        ImGui::SetNextItemWidth(-1);
        ImGui::BeginDisabled();
        char currentBuf[256] = {0};
        std::strncpy(currentBuf, mCurrentBranch.c_str(), sizeof(currentBuf) - 1);
        ImGui::InputText("##MergeCurrentBranch", currentBuf, sizeof(currentBuf), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();

        ImGui::Spacing();

        // Source branch picker
        ImGui::Text("Merge From");
        ImGui::SetNextItemWidth(-1);
        if (hasBranches)
        {
            if (ImGui::BeginCombo("##MergeSourceBranch", mBranchNames[mSelectedSourceBranch].c_str()))
            {
                for (int32_t i = 0; i < (int32_t)mBranchNames.size(); ++i)
                {
                    bool selected = (i == mSelectedSourceBranch);
                    if (ImGui::Selectable(mBranchNames[i].c_str(), selected))
                    {
                        mSelectedSourceBranch = i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::TextDisabled("No other branches available to merge");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox("Fast-forward only (--ff-only)", &mFastForwardOnly);

        ImGui::Spacing();

        // Show merge result if available
        if (!mMergeResult.empty())
        {
            bool isError = (mMergeResult.find("Error") != std::string::npos ||
                            mMergeResult.find("CONFLICT") != std::string::npos ||
                            mMergeResult.find("failed") != std::string::npos);

            if (isError)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
            }
            ImGui::TextWrapped("%s", mMergeResult.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (mMergeInProgress)
        {
            ImGui::TextDisabled("Merge in progress...");
        }

        ImGui::Separator();
        ImGui::Spacing();

        bool canMerge = hasBranches && !mMergeInProgress;
        if (!canMerge) ImGui::BeginDisabled();

        if (ImGui::Button("Merge", ImVec2(120, 0)))
        {
            mMergeInProgress = true;
            mMergeResult.clear();
            mCancelToken = CreateCancelToken();

            // Capture values for the thread
            std::string repoPath;
            GitService* service = GitService::Get();
            if (service && service->IsRepositoryOpen())
            {
                repoPath = service->GetCurrentRepo()->GetPath();
            }

            std::string sourceBranch = mBranchNames[mSelectedSourceBranch];
            bool ffOnly = mFastForwardOnly;
            GitCancelToken cancelToken = mCancelToken;

            // Run merge via git CLI on a detached thread
            std::thread mergeThread([this, repoPath, sourceBranch, ffOnly, cancelToken]()
            {
                std::vector<std::string> args;
                args.push_back("git");
                args.push_back("merge");

                if (ffOnly)
                {
                    args.push_back("--ff-only");
                }

                args.push_back(sourceBranch);

                std::string stdoutAccum;
                std::string stderrAccum;

                int32_t exitCode = GitProcessRunner::Run(
                    repoPath,
                    args,
                    [&stdoutAccum](const std::string& line) { stdoutAccum += line + "\n"; },
                    [&stderrAccum](const std::string& line) { stderrAccum += line + "\n"; },
                    cancelToken
                );

                if (cancelToken && cancelToken->load())
                {
                    mMergeResult = "Merge cancelled.";
                }
                else if (exitCode != 0)
                {
                    mMergeResult = "Error: merge failed (exit " + std::to_string(exitCode) + ").\n" + stderrAccum;
                }
                else
                {
                    mMergeResult = "Merge completed successfully.";
                    if (!stdoutAccum.empty())
                    {
                        mMergeResult += "\n" + stdoutAccum;
                    }
                }

                mMergeInProgress = false;

                // Refresh repository status after merge
                GitService* service = GitService::Get();
                if (service && service->IsRepositoryOpen())
                {
                    service->GetCurrentRepo()->RefreshStatus();
                }
            });

            mergeThread.detach();
        }

        if (!canMerge) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            if (mMergeInProgress && mCancelToken)
            {
                mCancelToken->store(true);
            }
            mIsOpen = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

#endif
