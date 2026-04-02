#include "InputMap.h"
#include "Input.h"


InputMap* InputMap::sInstance = nullptr;

void InputMap::Create()
{
    Destroy();
    sInstance = new InputMap();
}

void InputMap::Destroy()
{
    if (sInstance != nullptr)
    {
        delete sInstance;
        sInstance = nullptr;
    }
}

InputMap* InputMap::Get()
{
    return sInstance;
}

InputMap::InputMap()
{
    ResetToDefaults();
}

void InputMap::ResetToDefaults()
{
    // Clear all mappings
    for (int32_t i = 0; i < GAMEPAD_BUTTON_COUNT; ++i)
    {
        mButtonMappings[i] = -1;
    }
    for (int32_t i = 0; i < GAMEPAD_AXIS_COUNT; ++i)
    {
        mAxisPositiveKeys[i] = -1;
        mAxisNegativeKeys[i] = -1;
        mAxisUseMouseX[i] = false;
        mAxisUseMouseY[i] = false;
        mAxisMouseSensitivity[i] = 1.0f;
    }

    mMouseEnabled = true;
    mPointerEnabled = true;

    // Default button mappings
    mButtonMappings[GAMEPAD_A] = POLYPHASE_KEY_SPACE;
    mButtonMappings[GAMEPAD_B] = POLYPHASE_KEY_SHIFT_L;
    // GAMEPAD_C: unmapped (Wii-specific)
    mButtonMappings[GAMEPAD_X] = POLYPHASE_KEY_E;
    mButtonMappings[GAMEPAD_Y] = POLYPHASE_KEY_Q;
    // GAMEPAD_Z: unmapped (Wii-specific)
    mButtonMappings[GAMEPAD_L1] = POLYPHASE_KEY_1;
    mButtonMappings[GAMEPAD_R1] = POLYPHASE_KEY_2;
    mButtonMappings[GAMEPAD_L2] = POLYPHASE_KEY_3;
    mButtonMappings[GAMEPAD_R2] = POLYPHASE_KEY_4;
    mButtonMappings[GAMEPAD_THUMBL] = POLYPHASE_KEY_F;
    mButtonMappings[GAMEPAD_THUMBR] = POLYPHASE_KEY_R;
    mButtonMappings[GAMEPAD_START] = POLYPHASE_KEY_ENTER;
    mButtonMappings[GAMEPAD_SELECT] = POLYPHASE_KEY_BACKSPACE;
    mButtonMappings[GAMEPAD_LEFT] = POLYPHASE_KEY_LEFT;
    mButtonMappings[GAMEPAD_RIGHT] = POLYPHASE_KEY_RIGHT;
    mButtonMappings[GAMEPAD_UP] = POLYPHASE_KEY_UP;
    mButtonMappings[GAMEPAD_DOWN] = POLYPHASE_KEY_DOWN;
    mButtonMappings[GAMEPAD_HOME] = POLYPHASE_KEY_HOME;
    // Stick-as-button entries: unmapped by default (auto-derived from axes)

    // Default axis mappings
    mAxisNegativeKeys[GAMEPAD_AXIS_LTHUMB_X] = POLYPHASE_KEY_A;
    mAxisPositiveKeys[GAMEPAD_AXIS_LTHUMB_X] = POLYPHASE_KEY_D;
    mAxisNegativeKeys[GAMEPAD_AXIS_LTHUMB_Y] = POLYPHASE_KEY_W;
    mAxisPositiveKeys[GAMEPAD_AXIS_LTHUMB_Y] = POLYPHASE_KEY_S;
    mAxisNegativeKeys[GAMEPAD_AXIS_RTHUMB_X] = POLYPHASE_KEY_J;
    mAxisPositiveKeys[GAMEPAD_AXIS_RTHUMB_X] = POLYPHASE_KEY_L;
    mAxisNegativeKeys[GAMEPAD_AXIS_RTHUMB_Y] = POLYPHASE_KEY_I;
    mAxisPositiveKeys[GAMEPAD_AXIS_RTHUMB_Y] = POLYPHASE_KEY_K;
    mAxisPositiveKeys[GAMEPAD_AXIS_LTRIGGER] = POLYPHASE_KEY_Z;
    mAxisPositiveKeys[GAMEPAD_AXIS_RTRIGGER] = POLYPHASE_KEY_X;

    // Default mouse-to-axis: right stick driven by mouse
    mAxisUseMouseX[GAMEPAD_AXIS_RTHUMB_X] = true;
    mAxisUseMouseY[GAMEPAD_AXIS_RTHUMB_Y] = true;
}

// --- Button mappings ---

void InputMap::SetButtonMapping(GamepadButtonCode button, int32_t keyCode)
{
    if (button >= 0 && button < GAMEPAD_BUTTON_COUNT)
    {
        mButtonMappings[button] = keyCode;
    }
}

int32_t InputMap::GetButtonMapping(GamepadButtonCode button) const
{
    if (button >= 0 && button < GAMEPAD_BUTTON_COUNT)
    {
        return mButtonMappings[button];
    }
    return -1;
}

// --- Axis mappings ---

void InputMap::SetAxisMapping(GamepadAxisCode axis, int32_t positiveKey, int32_t negativeKey)
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
    {
        mAxisPositiveKeys[axis] = positiveKey;
        mAxisNegativeKeys[axis] = negativeKey;
    }
}

void InputMap::GetAxisMapping(GamepadAxisCode axis, int32_t& positiveKey, int32_t& negativeKey) const
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
    {
        positiveKey = mAxisPositiveKeys[axis];
        negativeKey = mAxisNegativeKeys[axis];
    }
    else
    {
        positiveKey = -1;
        negativeKey = -1;
    }
}

// --- Mouse-to-axis ---

void InputMap::SetAxisMouseMapping(GamepadAxisCode axis, bool useMouseX, bool useMouseY, float sensitivity)
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
    {
        mAxisUseMouseX[axis] = useMouseX;
        mAxisUseMouseY[axis] = useMouseY;
        mAxisMouseSensitivity[axis] = sensitivity;
    }
}

bool InputMap::GetAxisUsesMouseX(GamepadAxisCode axis) const
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
        return mAxisUseMouseX[axis];
    return false;
}

bool InputMap::GetAxisUsesMouseY(GamepadAxisCode axis) const
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
        return mAxisUseMouseY[axis];
    return false;
}

float InputMap::GetAxisMouseSensitivity(GamepadAxisCode axis) const
{
    if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
        return mAxisMouseSensitivity[axis];
    return 1.0f;
}

// --- Mouse/Pointer toggles ---

void InputMap::SetMouseEnabled(bool enabled) { mMouseEnabled = enabled; }
bool InputMap::IsMouseEnabled() const { return mMouseEnabled; }
void InputMap::SetPointerEnabled(bool enabled) { mPointerEnabled = enabled; }
bool InputMap::IsPointerEnabled() const { return mPointerEnabled; }

// --- Keyboard query helpers ---

bool InputMap::IsButtonDownViaKeyboard(GamepadButtonCode button) const
{
    if (button >= 0 && button < GAMEPAD_BUTTON_COUNT)
    {
        int32_t key = mButtonMappings[button];
        if (key >= 0)
        {
            return INP_IsKeyDown(key);
        }
    }
    return false;
}

bool InputMap::IsButtonJustDownViaKeyboard(GamepadButtonCode button) const
{
    if (button >= 0 && button < GAMEPAD_BUTTON_COUNT)
    {
        int32_t key = mButtonMappings[button];
        if (key >= 0)
        {
            return INP_IsKeyJustDown(key);
        }
    }
    return false;
}

bool InputMap::IsButtonJustUpViaKeyboard(GamepadButtonCode button) const
{
    if (button >= 0 && button < GAMEPAD_BUTTON_COUNT)
    {
        int32_t key = mButtonMappings[button];
        if (key >= 0)
        {
            return INP_IsKeyJustUp(key);
        }
    }
    return false;
}

float InputMap::GetAxisValueFromMapping(GamepadAxisCode axis) const
{
    if (axis < 0 || axis >= GAMEPAD_AXIS_COUNT)
        return 0.0f;

    float value = 0.0f;

    // Keyboard keys
    int32_t posKey = mAxisPositiveKeys[axis];
    int32_t negKey = mAxisNegativeKeys[axis];

    if (posKey >= 0 && INP_IsKeyDown(posKey))
        value += 1.0f;
    if (negKey >= 0 && INP_IsKeyDown(negKey))
        value -= 1.0f;

    // Mouse delta -> axis (reads raw delta directly from InputState)
    if (mAxisUseMouseX[axis])
    {
        int32_t dx = 0;
        int32_t dy = 0;
        INP_GetMouseDelta(dx, dy);
        float sensitivity = mAxisMouseSensitivity[axis];
        value += dx * sensitivity * 0.01f;
    }
    if (mAxisUseMouseY[axis])
    {
        int32_t dx = 0;
        int32_t dy = 0;
        INP_GetMouseDelta(dx, dy);
        float sensitivity = mAxisMouseSensitivity[axis];
        value += dy * sensitivity * 0.01f;
    }

    // Clamp to [-1, 1]
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;

    return value;
}

// --- Display string helpers ---

const char* InputMap::GetKeyCodeName(int32_t keyCode)
{
    switch (keyCode)
    {
    case POLYPHASE_KEY_A: return "A";
    case POLYPHASE_KEY_B: return "B";
    case POLYPHASE_KEY_C: return "C";
    case POLYPHASE_KEY_D: return "D";
    case POLYPHASE_KEY_E: return "E";
    case POLYPHASE_KEY_F: return "F";
    case POLYPHASE_KEY_G: return "G";
    case POLYPHASE_KEY_H: return "H";
    case POLYPHASE_KEY_I: return "I";
    case POLYPHASE_KEY_J: return "J";
    case POLYPHASE_KEY_K: return "K";
    case POLYPHASE_KEY_L: return "L";
    case POLYPHASE_KEY_M: return "M";
    case POLYPHASE_KEY_N: return "N";
    case POLYPHASE_KEY_O: return "O";
    case POLYPHASE_KEY_P: return "P";
    case POLYPHASE_KEY_Q: return "Q";
    case POLYPHASE_KEY_R: return "R";
    case POLYPHASE_KEY_S: return "S";
    case POLYPHASE_KEY_T: return "T";
    case POLYPHASE_KEY_U: return "U";
    case POLYPHASE_KEY_V: return "V";
    case POLYPHASE_KEY_W: return "W";
    case POLYPHASE_KEY_X: return "X";
    case POLYPHASE_KEY_Y: return "Y";
    case POLYPHASE_KEY_Z: return "Z";
    case POLYPHASE_KEY_0: return "0";
    case POLYPHASE_KEY_1: return "1";
    case POLYPHASE_KEY_2: return "2";
    case POLYPHASE_KEY_3: return "3";
    case POLYPHASE_KEY_4: return "4";
    case POLYPHASE_KEY_5: return "5";
    case POLYPHASE_KEY_6: return "6";
    case POLYPHASE_KEY_7: return "7";
    case POLYPHASE_KEY_8: return "8";
    case POLYPHASE_KEY_9: return "9";
    case POLYPHASE_KEY_SPACE: return "Space";
    case POLYPHASE_KEY_ENTER: return "Enter";
    case POLYPHASE_KEY_BACKSPACE: return "Backspace";
    case POLYPHASE_KEY_TAB: return "Tab";
    case POLYPHASE_KEY_ESCAPE: return "Escape";
    case POLYPHASE_KEY_SHIFT_L: return "Left Shift";
    case POLYPHASE_KEY_CONTROL_L: return "Left Ctrl";
    case POLYPHASE_KEY_ALT_L: return "Left Alt";
    case POLYPHASE_KEY_UP: return "Up";
    case POLYPHASE_KEY_DOWN: return "Down";
    case POLYPHASE_KEY_LEFT: return "Left";
    case POLYPHASE_KEY_RIGHT: return "Right";
    case POLYPHASE_KEY_INSERT: return "Insert";
    case POLYPHASE_KEY_DELETE: return "Delete";
    case POLYPHASE_KEY_HOME: return "Home";
    case POLYPHASE_KEY_END: return "End";
    case POLYPHASE_KEY_PAGE_UP: return "Page Up";
    case POLYPHASE_KEY_PAGE_DOWN: return "Page Down";
    case POLYPHASE_KEY_F1: return "F1";
    case POLYPHASE_KEY_F2: return "F2";
    case POLYPHASE_KEY_F3: return "F3";
    case POLYPHASE_KEY_F4: return "F4";
    case POLYPHASE_KEY_F5: return "F5";
    case POLYPHASE_KEY_F6: return "F6";
    case POLYPHASE_KEY_F7: return "F7";
    case POLYPHASE_KEY_F8: return "F8";
    case POLYPHASE_KEY_F9: return "F9";
    case POLYPHASE_KEY_F10: return "F10";
    case POLYPHASE_KEY_F11: return "F11";
    case POLYPHASE_KEY_F12: return "F12";
    case POLYPHASE_KEY_NUMPAD0: return "Numpad 0";
    case POLYPHASE_KEY_NUMPAD1: return "Numpad 1";
    case POLYPHASE_KEY_NUMPAD2: return "Numpad 2";
    case POLYPHASE_KEY_NUMPAD3: return "Numpad 3";
    case POLYPHASE_KEY_NUMPAD4: return "Numpad 4";
    case POLYPHASE_KEY_NUMPAD5: return "Numpad 5";
    case POLYPHASE_KEY_NUMPAD6: return "Numpad 6";
    case POLYPHASE_KEY_NUMPAD7: return "Numpad 7";
    case POLYPHASE_KEY_NUMPAD8: return "Numpad 8";
    case POLYPHASE_KEY_NUMPAD9: return "Numpad 9";
    case POLYPHASE_KEY_PERIOD: return "Period";
    case POLYPHASE_KEY_COMMA: return "Comma";
    case POLYPHASE_KEY_PLUS: return "Plus";
    case POLYPHASE_KEY_MINUS: return "Minus";
    case POLYPHASE_KEY_COLON: return "Semicolon";
    case POLYPHASE_KEY_QUESTION: return "Slash";
    case POLYPHASE_KEY_SQUIGGLE: return "Tilde";
    case POLYPHASE_KEY_LEFT_BRACKET: return "Left Bracket";
    case POLYPHASE_KEY_BACK_SLASH: return "Backslash";
    case POLYPHASE_KEY_RIGHT_BRACKET: return "Right Bracket";
    case POLYPHASE_KEY_QUOTE: return "Quote";
    case POLYPHASE_KEY_DECIMAL: return "Decimal";
    default: return "—";
    }
}

const char* InputMap::GetGamepadButtonName(GamepadButtonCode button)
{
    switch (button)
    {
    case GAMEPAD_A: return "Gamepad.A";
    case GAMEPAD_B: return "Gamepad.B";
    case GAMEPAD_C: return "Gamepad.C";
    case GAMEPAD_X: return "Gamepad.X";
    case GAMEPAD_Y: return "Gamepad.Y";
    case GAMEPAD_Z: return "Gamepad.Z";
    case GAMEPAD_L1: return "Gamepad.L1";
    case GAMEPAD_R1: return "Gamepad.R1";
    case GAMEPAD_L2: return "Gamepad.L2";
    case GAMEPAD_R2: return "Gamepad.R2";
    case GAMEPAD_THUMBL: return "Gamepad.L3";
    case GAMEPAD_THUMBR: return "Gamepad.R3";
    case GAMEPAD_START: return "Gamepad.Start";
    case GAMEPAD_SELECT: return "Gamepad.Select";
    case GAMEPAD_LEFT: return "Gamepad.Left";
    case GAMEPAD_RIGHT: return "Gamepad.Right";
    case GAMEPAD_UP: return "Gamepad.Up";
    case GAMEPAD_DOWN: return "Gamepad.Down";
    case GAMEPAD_L_LEFT: return "Gamepad.LsLeft";
    case GAMEPAD_L_RIGHT: return "Gamepad.LsRight";
    case GAMEPAD_L_UP: return "Gamepad.LsUp";
    case GAMEPAD_L_DOWN: return "Gamepad.LsDown";
    case GAMEPAD_R_LEFT: return "Gamepad.RsLeft";
    case GAMEPAD_R_RIGHT: return "Gamepad.RsRight";
    case GAMEPAD_R_UP: return "Gamepad.RsUp";
    case GAMEPAD_R_DOWN: return "Gamepad.RsDown";
    case GAMEPAD_HOME: return "Gamepad.Home";
    default: return "Unknown";
    }
}

const char* InputMap::GetGamepadAxisName(GamepadAxisCode axis)
{
    switch (axis)
    {
    case GAMEPAD_AXIS_LTRIGGER: return "Left Trigger";
    case GAMEPAD_AXIS_RTRIGGER: return "Right Trigger";
    case GAMEPAD_AXIS_LTHUMB_X: return "Left Stick X";
    case GAMEPAD_AXIS_LTHUMB_Y: return "Left Stick Y";
    case GAMEPAD_AXIS_RTHUMB_X: return "Right Stick X";
    case GAMEPAD_AXIS_RTHUMB_Y: return "Right Stick Y";
    default: return "Unknown";
    }
}
