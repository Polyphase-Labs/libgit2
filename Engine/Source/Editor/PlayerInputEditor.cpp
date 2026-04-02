#if EDITOR

#include "PlayerInputEditor.h"
#include "Input/PlayerInputSystem.h"
#include "Input/InputMap.h"
#include "Input/Input.h"

#include "imgui.h"

static PlayerInputEditor sPlayerInputEditor;

PlayerInputEditor* GetPlayerInputEditor()
{
    return &sPlayerInputEditor;
}

void PlayerInputEditor::Open()
{
    mIsOpen = true;
}

bool PlayerInputEditor::IsOpen() const
{
    return mIsOpen;
}

static const char* TriggerModeNames[] = { "SinglePress", "Hold", "KeepHeld", "MultiPress" };
static const char* SourceTypeNames[] = { "Keyboard", "MouseButton", "GamepadButton", "GamepadAxis", "Pointer" };
static const char* AxisDirNames[] = { "Positive", "Negative", "Full" };

// Key name entries for dropdown
struct KeyEntry { int code; const char* name; };

static const KeyEntry sKeyEntries[] = {
    { POLYPHASE_KEY_A, "A" }, { POLYPHASE_KEY_B, "B" }, { POLYPHASE_KEY_C, "C" },
    { POLYPHASE_KEY_D, "D" }, { POLYPHASE_KEY_E, "E" }, { POLYPHASE_KEY_F, "F" },
    { POLYPHASE_KEY_G, "G" }, { POLYPHASE_KEY_H, "H" }, { POLYPHASE_KEY_I, "I" },
    { POLYPHASE_KEY_J, "J" }, { POLYPHASE_KEY_K, "K" }, { POLYPHASE_KEY_L, "L" },
    { POLYPHASE_KEY_M, "M" }, { POLYPHASE_KEY_N, "N" }, { POLYPHASE_KEY_O, "O" },
    { POLYPHASE_KEY_P, "P" }, { POLYPHASE_KEY_Q, "Q" }, { POLYPHASE_KEY_R, "R" },
    { POLYPHASE_KEY_S, "S" }, { POLYPHASE_KEY_T, "T" }, { POLYPHASE_KEY_U, "U" },
    { POLYPHASE_KEY_V, "V" }, { POLYPHASE_KEY_W, "W" }, { POLYPHASE_KEY_X, "X" },
    { POLYPHASE_KEY_Y, "Y" }, { POLYPHASE_KEY_Z, "Z" },
    { POLYPHASE_KEY_0, "0" }, { POLYPHASE_KEY_1, "1" }, { POLYPHASE_KEY_2, "2" },
    { POLYPHASE_KEY_3, "3" }, { POLYPHASE_KEY_4, "4" }, { POLYPHASE_KEY_5, "5" },
    { POLYPHASE_KEY_6, "6" }, { POLYPHASE_KEY_7, "7" }, { POLYPHASE_KEY_8, "8" },
    { POLYPHASE_KEY_9, "9" },
    { POLYPHASE_KEY_SPACE, "Space" }, { POLYPHASE_KEY_ENTER, "Enter" },
    { POLYPHASE_KEY_ESCAPE, "Escape" }, { POLYPHASE_KEY_TAB, "Tab" },
    { POLYPHASE_KEY_BACKSPACE, "Backspace" },
    { POLYPHASE_KEY_SHIFT_L, "Left Shift" }, { POLYPHASE_KEY_CONTROL_L, "Left Ctrl" },
    { POLYPHASE_KEY_ALT_L, "Left Alt" },
    { POLYPHASE_KEY_UP, "Up" }, { POLYPHASE_KEY_DOWN, "Down" },
    { POLYPHASE_KEY_LEFT, "Left" }, { POLYPHASE_KEY_RIGHT, "Right" },
    { POLYPHASE_KEY_F1, "F1" }, { POLYPHASE_KEY_F2, "F2" }, { POLYPHASE_KEY_F3, "F3" },
    { POLYPHASE_KEY_F4, "F4" }, { POLYPHASE_KEY_F5, "F5" }, { POLYPHASE_KEY_F6, "F6" },
    { POLYPHASE_KEY_F7, "F7" }, { POLYPHASE_KEY_F8, "F8" }, { POLYPHASE_KEY_F9, "F9" },
    { POLYPHASE_KEY_F10, "F10" }, { POLYPHASE_KEY_F11, "F11" }, { POLYPHASE_KEY_F12, "F12" },
    { POLYPHASE_KEY_INSERT, "Insert" }, { POLYPHASE_KEY_DELETE, "Delete" },
    { POLYPHASE_KEY_HOME, "Home" }, { POLYPHASE_KEY_END, "End" },
    { POLYPHASE_KEY_PAGE_UP, "Page Up" }, { POLYPHASE_KEY_PAGE_DOWN, "Page Down" },
    { POLYPHASE_KEY_PERIOD, "Period" }, { POLYPHASE_KEY_COMMA, "Comma" },
    { POLYPHASE_KEY_PLUS, "Plus" }, { POLYPHASE_KEY_MINUS, "Minus" },
};
static const int sKeyEntryCount = sizeof(sKeyEntries) / sizeof(sKeyEntries[0]);

static const KeyEntry sMouseEntries[] = {
    { MOUSE_LEFT, "Mouse Left" }, { MOUSE_RIGHT, "Mouse Right" },
    { MOUSE_MIDDLE, "Mouse Middle" }, { MOUSE_X1, "Mouse X1" }, { MOUSE_X2, "Mouse X2" },
};
static const int sMouseEntryCount = sizeof(sMouseEntries) / sizeof(sMouseEntries[0]);

static const KeyEntry sGamepadBtnEntries[] = {
    { GAMEPAD_A, "A (Cross)" }, { GAMEPAD_B, "B (Circle)" },
    { GAMEPAD_C, "C" },
    { GAMEPAD_X, "X (Square)" }, { GAMEPAD_Y, "Y (Triangle)" },
    { GAMEPAD_Z, "Z" },
    { GAMEPAD_L1, "L1" }, { GAMEPAD_R1, "R1" },
    { GAMEPAD_L2, "L2" }, { GAMEPAD_R2, "R2" },
    { GAMEPAD_THUMBL, "L3" }, { GAMEPAD_THUMBR, "R3" },
    { GAMEPAD_START, "Start" }, { GAMEPAD_SELECT, "Select / Back" },
    { GAMEPAD_HOME, "Home" },
    { GAMEPAD_UP, "D-Pad Up" }, { GAMEPAD_DOWN, "D-Pad Down" },
    { GAMEPAD_LEFT, "D-Pad Left" }, { GAMEPAD_RIGHT, "D-Pad Right" },
    { GAMEPAD_L_LEFT, "LStick Left" }, { GAMEPAD_L_RIGHT, "LStick Right" },
    { GAMEPAD_L_UP, "LStick Up" }, { GAMEPAD_L_DOWN, "LStick Down" },
    { GAMEPAD_R_LEFT, "RStick Left" }, { GAMEPAD_R_RIGHT, "RStick Right" },
    { GAMEPAD_R_UP, "RStick Up" }, { GAMEPAD_R_DOWN, "RStick Down" },
};
static const int sGamepadBtnEntryCount = sizeof(sGamepadBtnEntries) / sizeof(sGamepadBtnEntries[0]);

static const KeyEntry sGamepadAxisEntries[] = {
    { GAMEPAD_AXIS_LTHUMB_X, "Left Stick X" }, { GAMEPAD_AXIS_LTHUMB_Y, "Left Stick Y" },
    { GAMEPAD_AXIS_RTHUMB_X, "Right Stick X" }, { GAMEPAD_AXIS_RTHUMB_Y, "Right Stick Y" },
    { GAMEPAD_AXIS_LTRIGGER, "Left Trigger" }, { GAMEPAD_AXIS_RTRIGGER, "Right Trigger" },
};
static const int sGamepadAxisEntryCount = sizeof(sGamepadAxisEntries) / sizeof(sGamepadAxisEntries[0]);

static const KeyEntry sPointerEntries[] = {
    { 0, "Pointer 0 (Mouse/Touch)" },
    { 1, "Pointer 1" },
    { 2, "Pointer 2" },
    { 3, "Pointer 3" },
};
static const int sPointerEntryCount = sizeof(sPointerEntries) / sizeof(sPointerEntries[0]);

// Find index in entry array by code, returns -1 if not found
static int FindEntryIndex(const KeyEntry* entries, int count, int code)
{
    for (int i = 0; i < count; ++i)
    {
        if (entries[i].code == code) return i;
    }
    return -1;
}

void PlayerInputEditor::Draw()
{
    if (!mIsOpen)
        return;

    PlayerInputSystem* sys = PlayerInputSystem::Get();
    if (sys == nullptr)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 windowSize(700.0f, 550.0f);
    ImVec2 windowPos((io.DisplaySize.x - windowSize.x) * 0.5f, (io.DisplaySize.y - windowSize.y) * 0.5f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Player Input Editor", &mIsOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (mCapturing)
        {
            DrawCaptureOverlay();
        }

        // Split: left = action list, right = details
        ImGui::Columns(2, "##player_input_cols", true);
        ImGui::SetColumnWidth(0, 250.0f);

        DrawActionList();
        ImGui::NextColumn();
        DrawActionDetails();

        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save to Project"))
        {
            sys->SaveProjectActions();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload from Project"))
        {
            sys->LoadProjectActions();
            mSelectedActionIndex = -1;
        }
    }
    ImGui::End();
}

void PlayerInputEditor::DrawActionList()
{
    PlayerInputSystem* sys = PlayerInputSystem::Get();
    const auto& actions = sys->GetActions();

    ImGui::Text("Actions");
    ImGui::Separator();

    // Group by category
    std::string lastCategory;
    bool lastCategoryOpen = false;
    for (int i = 0; i < (int)actions.size(); ++i)
    {
        const auto& action = actions[i];

        if (action.category != lastCategory)
        {
            // Close previous category tree node
            if (!lastCategory.empty() && lastCategoryOpen)
                ImGui::TreePop();

            lastCategoryOpen = ImGui::TreeNodeEx(action.category.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            lastCategory = action.category;

            if (!lastCategoryOpen)
            {
                // Skip actions in this closed category
                while (i + 1 < (int)actions.size() && actions[i + 1].category == lastCategory)
                    ++i;
                continue;
            }
        }

        if (!lastCategoryOpen)
            continue;

        bool selected = (mSelectedActionIndex == i);
        if (ImGui::Selectable(action.name.c_str(), selected))
        {
            mSelectedActionIndex = i;
        }
    }
    if (!lastCategory.empty() && lastCategoryOpen)
        ImGui::TreePop();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add new action
    ImGui::Text("Add Action:");
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("Category", mNewCategory, sizeof(mNewCategory));
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("Name", mNewActionName, sizeof(mNewActionName));

    bool canAdd = mNewCategory[0] != '\0' && mNewActionName[0] != '\0';
    if (!canAdd) ImGui::BeginDisabled();
    if (ImGui::Button("Add"))
    {
        sys->RegisterAction(mNewCategory, mNewActionName);
        mNewCategory[0] = '\0';
        mNewActionName[0] = '\0';
    }
    if (!canAdd) ImGui::EndDisabled();
}

void PlayerInputEditor::DrawActionDetails()
{
    PlayerInputSystem* sys = PlayerInputSystem::Get();
    const auto& actions = sys->GetActions();

    if (mSelectedActionIndex < 0 || mSelectedActionIndex >= (int)actions.size())
    {
        ImGui::TextDisabled("Select an action to edit.");
        return;
    }

    // Need non-const access
    InputAction* action = sys->FindAction(actions[mSelectedActionIndex].category,
                                          actions[mSelectedActionIndex].name);
    if (action == nullptr) return;

    ImGui::Text("Action: %s/%s", action->category.c_str(), action->name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Trigger mode
    int modeIdx = (int)action->trigger.mode;
    ImGui::Text("Trigger Mode:");
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::Combo("##trigger_mode", &modeIdx, TriggerModeNames, 4))
    {
        action->trigger.mode = (TriggerMode)modeIdx;
    }

    // Trigger-specific params
    if (action->trigger.mode == TriggerMode::Hold)
    {
        ImGui::SetNextItemWidth(150.0f);
        ImGui::DragFloat("Hold Duration (s)", &action->trigger.holdDuration, 0.05f, 0.1f, 10.0f, "%.2f");
    }
    else if (action->trigger.mode == TriggerMode::MultiPress)
    {
        ImGui::SetNextItemWidth(150.0f);
        ImGui::DragInt("Press Count", &action->trigger.multiPressCount, 1, 2, 10);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::DragFloat("Press Window (s)", &action->trigger.multiPressWindow, 0.01f, 0.1f, 2.0f, "%.2f");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Bindings:");
    ImGui::Spacing();

    // Bindings list
    for (int b = 0; b < (int)action->bindings.size(); ++b)
    {
        auto& binding = action->bindings[b];
        ImGui::PushID(b);

        // Source type dropdown
        int srcIdx = (int)binding.sourceType;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::Combo("##src", &srcIdx, SourceTypeNames, 5))
        {
            binding.sourceType = (InputSourceType)srcIdx;
            binding.code = 0;
        }
        ImGui::SameLine();

        // Code dropdown (enum selector based on source type)
        {
            const KeyEntry* entries = nullptr;
            int entryCount = 0;

            if (binding.sourceType == InputSourceType::Keyboard)
            { entries = sKeyEntries; entryCount = sKeyEntryCount; }
            else if (binding.sourceType == InputSourceType::MouseButton)
            { entries = sMouseEntries; entryCount = sMouseEntryCount; }
            else if (binding.sourceType == InputSourceType::GamepadButton)
            { entries = sGamepadBtnEntries; entryCount = sGamepadBtnEntryCount; }
            else if (binding.sourceType == InputSourceType::GamepadAxis)
            { entries = sGamepadAxisEntries; entryCount = sGamepadAxisEntryCount; }
            else if (binding.sourceType == InputSourceType::Pointer)
            { entries = sPointerEntries; entryCount = sPointerEntryCount; }

            if (entries != nullptr)
            {
                int selectedIdx = FindEntryIndex(entries, entryCount, binding.code);
                const char* preview = (selectedIdx >= 0) ? entries[selectedIdx].name : "---";

                ImGui::SetNextItemWidth(130.0f);
                if (ImGui::BeginCombo("##code", preview))
                {
                    for (int e = 0; e < entryCount; ++e)
                    {
                        bool selected = (entries[e].code == binding.code);
                        if (ImGui::Selectable(entries[e].name, selected))
                        {
                            binding.code = entries[e].code;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }

        ImGui::SameLine();
        if (!mCapturing && ImGui::SmallButton("Remap"))
        {
            mCapturing = true;
            mCaptureBindingSlot = b;
            mCaptureTimer = 5.0f;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("X"))
        {
            action->bindings.erase(action->bindings.begin() + b);
            --b;
            ImGui::PopID();
            continue;
        }

        // Modifiers
        ImGui::SameLine();
        ImGui::Checkbox("Ctrl", &binding.requireCtrl);
        ImGui::SameLine();
        ImGui::Checkbox("Shift", &binding.requireShift);
        ImGui::SameLine();
        ImGui::Checkbox("Alt", &binding.requireAlt);

        // Axis-specific
        if (binding.sourceType == InputSourceType::GamepadAxis)
        {
            int dirIdx = (int)binding.axisDirection;
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::Combo("##dir", &dirIdx, AxisDirNames, 3))
            {
                binding.axisDirection = (AxisDirection)dirIdx;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragFloat("##thresh", &binding.axisThreshold, 0.01f, 0.1f, 1.0f, "%.2f");
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    if (!mCapturing && ImGui::Button("+ Add Binding"))
    {
        InputActionBinding newBinding;
        action->bindings.push_back(newBinding);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Delete Action"))
    {
        sys->UnregisterAction(action->category, action->name);
        mSelectedActionIndex = -1;
    }
}

void PlayerInputEditor::DrawCaptureOverlay()
{
    mCaptureTimer -= ImGui::GetIO().DeltaTime;

    // Timeout only — no escape cancel so Escape can be bound
    if (mCaptureTimer <= 0.0f)
    {
        mCapturing = false;
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    ImGui::Text("Press a key, mouse button, or gamepad button... (%.0fs)", mCaptureTimer);
    ImGui::PopStyleColor();

    PlayerInputSystem* sys = PlayerInputSystem::Get();
    const auto& actions = sys->GetActions();
    if (mSelectedActionIndex < 0 || mSelectedActionIndex >= (int)actions.size())
    {
        mCapturing = false;
        return;
    }

    InputAction* action = sys->FindAction(actions[mSelectedActionIndex].category,
                                          actions[mSelectedActionIndex].name);
    if (action == nullptr || mCaptureBindingSlot < 0 || mCaptureBindingSlot >= (int)action->bindings.size())
    {
        mCapturing = false;
        return;
    }

    auto& binding = action->bindings[mCaptureBindingSlot];

    // Check keyboard (all keys including Escape)
    for (int key = 0; key < INPUT_MAX_KEYS; ++key)
    {
        // Skip modifier keys — they are recorded as modifiers, not primary keys
        if (key == POLYPHASE_KEY_SHIFT_L || key == POLYPHASE_KEY_SHIFT_R ||
            key == POLYPHASE_KEY_CONTROL_L || key == POLYPHASE_KEY_CONTROL_R ||
            key == POLYPHASE_KEY_ALT_L || key == POLYPHASE_KEY_ALT_R)
            continue;

        if (INP_IsKeyJustDown(key))
        {
            binding.sourceType = InputSourceType::Keyboard;
            binding.code = key;
            binding.requireCtrl = INP_IsKeyDown(POLYPHASE_KEY_CONTROL_L) || INP_IsKeyDown(POLYPHASE_KEY_CONTROL_R);
            binding.requireShift = INP_IsKeyDown(POLYPHASE_KEY_SHIFT_L) || INP_IsKeyDown(POLYPHASE_KEY_SHIFT_R);
            binding.requireAlt = INP_IsKeyDown(POLYPHASE_KEY_ALT_L) || INP_IsKeyDown(POLYPHASE_KEY_ALT_R);
            mCapturing = false;
            return;
        }
    }

    // Check mouse buttons
    for (int mb = 0; mb < MOUSE_BUTTON_COUNT; ++mb)
    {
        if (INP_IsMouseButtonJustDown(mb))
        {
            binding.sourceType = InputSourceType::MouseButton;
            binding.code = mb;
            mCapturing = false;
            return;
        }
    }

    // Check gamepad buttons
    for (int gb = 0; gb < GAMEPAD_BUTTON_COUNT; ++gb)
    {
        if (INP_IsGamepadButtonJustDown(gb, 0))
        {
            binding.sourceType = InputSourceType::GamepadButton;
            binding.code = gb;
            binding.gamepadIndex = 0;
            mCapturing = false;
            return;
        }
    }

    // Check gamepad axes
    for (int ax = 0; ax < GAMEPAD_AXIS_COUNT; ++ax)
    {
        float val = INP_GetGamepadAxisValue(ax, 0);
        if (val > 0.7f)
        {
            binding.sourceType = InputSourceType::GamepadAxis;
            binding.code = ax;
            binding.axisDirection = AxisDirection::Positive;
            binding.gamepadIndex = 0;
            mCapturing = false;
            return;
        }
        else if (val < -0.7f)
        {
            binding.sourceType = InputSourceType::GamepadAxis;
            binding.code = ax;
            binding.axisDirection = AxisDirection::Negative;
            binding.gamepadIndex = 0;
            mCapturing = false;
            return;
        }
    }
}

#endif
