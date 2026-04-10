#pragma once

#if EDITOR

#include "../GitTypes.h"
#include <string>
#include <vector>
#include <cstdint>

class GitMergeDialog
{
public:
    void Open();
    void Draw();

private:
    bool mIsOpen = false;
    bool mJustOpened = false;

    std::vector<std::string> mBranchNames;
    std::string mCurrentBranch;

    int32_t mSelectedSourceBranch = 0;
    bool mFastForwardOnly = false;

    // Merge runs via GitProcessRunner on the operation queue
    bool mMergeInProgress = false;
    std::string mMergeResult;
    GitCancelToken mCancelToken;
};

GitMergeDialog* GetGitMergeDialog();

#endif
