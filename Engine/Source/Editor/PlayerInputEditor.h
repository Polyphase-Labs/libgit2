#pragma once

#if EDITOR

#include <stdint.h>
#include <string>
#include <vector>

class PlayerInputEditor
{
public:
    void Open();
    void Draw();
    bool IsOpen() const;

private:

    bool mIsOpen = false;
    int mSelectedActionIndex = -1;
    char mNewCategory[64] = {};
    char mNewActionName[64] = {};

    // Capture state for binding
    bool mCapturing = false;
    int mCaptureBindingSlot = -1;
    float mCaptureTimer = 0.0f;

    void DrawActionList();
    void DrawActionDetails();
    void DrawCaptureOverlay();
};

PlayerInputEditor* GetPlayerInputEditor();

#endif
