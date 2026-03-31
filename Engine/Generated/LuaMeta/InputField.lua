--- @meta

---@class InputField : Widget
InputField = {}

---@param text string
function InputField:SetText(text) end

---@return string
function InputField:GetText() end

---@param placeholder string
function InputField:SetPlaceholder(placeholder) end

---@return string
function InputField:GetPlaceholder() end

---@param pos integer
function InputField:SetCaretPosition(pos) end

---@return integer
function InputField:GetCaretPosition() end

function InputField:SelectAll() end

function InputField:ClearSelection() end

---@return boolean
function InputField:HasSelection() end

---@return string
function InputField:GetSelectedText() end

function InputField:DeleteSelection() end

---@param start integer
---@param end integer
function InputField:Select(start, end) end

---@return integer
function InputField:GetSelectionStart() end

---@return integer
function InputField:GetSelectionEnd() end

---@param focused boolean
function InputField:SetFocused(focused) end

---@return boolean
function InputField:IsFocused() end

---@param enabled boolean
function InputField:SetPasswordMode(enabled) end

---@return boolean
function InputField:IsPasswordMode() end

---@param maxLen integer
function InputField:SetMaxLength(maxLen) end

---@return integer
function InputField:GetMaxLength() end

---@param editable boolean
function InputField:SetEditable(editable) end

---@return boolean
function InputField:IsEditable() end

---@param color Vector
function InputField:SetBackgroundColor(color) end

---@return Vector
function InputField:GetBackgroundColor() end

---@param color Vector
function InputField:SetFocusedBackgroundColor(color) end

---@return Vector
function InputField:GetFocusedBackgroundColor() end

---@param color Vector
function InputField:SetTextColor(color) end

---@return Vector
function InputField:GetTextColor() end

---@param color Vector
function InputField:SetPlaceholderColor(color) end

---@return Vector
function InputField:GetPlaceholderColor() end

---@param color Vector
function InputField:SetCaretColor(color) end

---@return Vector
function InputField:GetCaretColor() end

---@param color Vector
function InputField:SetSelectionColor(color) end

---@return Vector
function InputField:GetSelectionColor() end

---@param padding number
function InputField:SetTextPadding(padding) end

---@return number
function InputField:GetTextPadding() end

---@return Node
function InputField:GetBackground() end

---@return Node
function InputField:GetTextWidget() end

---@return Node
function InputField:GetCaret() end
